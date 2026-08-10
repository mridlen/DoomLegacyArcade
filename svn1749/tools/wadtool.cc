// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright (C) 2004-2020 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------

// Wadtool is a simple command line tool for manipulating WAD files:
// listing and extracting lumps, and creating new WAD files.
// Legacy uses it e.g. to build legacy.wad during packaging.
// The trunk/resources directory contains the inventory file for legacy.wad,
// legacy.wad.inventory, as well as various legacy.wad lumps as text and binary files.

#include <string>
#include <vector>

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef HAVE_MD5
#include "md5.h"
#endif

// Wadtool version number. Not the same as Doom Legacy version, or legacy.wad version.
#define VERSION "0.7.0"


// WAD files are always little-endian.
static inline int16_t SWAP_INT16(uint16_t x)
{
    return (int16_t)
     (  (( x & (uint16_t)0x00ffU) << 8)
      | (( x & (uint16_t)0xff00U) >> 8)
     );
}

static inline int32_t SWAP_INT32(uint32_t x)
{
    return (int32_t)
     (  (( x & (uint32_t)0x000000ffUL) << 24)
      | (( x & (uint32_t)0x0000ff00UL) <<  8)
      | (( x & (uint32_t)0x00ff0000UL) >>  8)
      | (( x & (uint32_t)0xff000000UL) >> 24)
     );
}

#ifdef __BIG_ENDIAN__
# define LE_SWAP16(x)  SWAP_INT16(x)
# define LE_SWAP32(x)  SWAP_INT32(x)
#else // little-endian machine
# define LE_SWAP16(x)  (x)
# define LE_SWAP32(x)  (x)
#endif

using namespace std;


static bool TestPadding(char *name, int len)
{
  // TEST padding of lumpnames
  bool warn = false;
  for (int j=0; j<len; j++)
    if (name[j] == 0)
    {
      // make sure the rest of the name is all NULs
      for (j++; j<len; j++)
        if (name[j] != 0)
        {
          name[j] = 0; // fix it
          warn = true;
        }

      break;
    }

  if (warn)
    printf("Warning: Lumpname %s was not padded with NULs!\n", name);

  return warn;
}


// WAD header
struct wadheader_t 
{
  union
  {
    char magic[4];   // "IWAD", "PWAD"
    int32_t imagic;
  };
  int32_t  numentries; // number of entries in WAD
  int32_t  diroffset;  // offset to WAD directory
};
static_assert (sizeof(wadheader_t) == 12, "wadheader_t is the wrong size!");


// WAD directory entry
struct waddir_t
{
  int32_t  offset;  // file offset of the resource
  int32_t  size;    // size of the resource
  union
  {
    char name[8]; // name of the resource (NUL-padded)
    int32_t  iname[2];
  };
};
static_assert (sizeof(waddir_t) == 16, "waddirr_t is the wrong size!");


// Simplified WAD class for wadtool
class Wad
{
protected:
  string filename; // the name of the associated physical file
  FILE *stream;    // associated stream
  int   diroffset; // offset to file directory
  int   numitems;  // number of data items (lumps)

  struct waddir_t *directory;  // wad directory

public:
#ifdef HAVE_MD5
  unsigned char md5sum[16];    // checksum for data integrity checks
#endif

  // constructor and destructor
  Wad();
  ~Wad();

  // open an existing wadfile
  bool Open(const char *fname);

  // create a new wadfile from lumps
  int Create(const char *wadname, const vector<string> &lump_names, const vector<string> &lump_filenames, const string &lump_dir);
  
  // query data item properties
  int GetNumItems() { return numitems; }
  const char *GetItemName(int i) { return directory[i].name; }
  int GetItemSize(int i) { return directory[i].size; }
  void ListItems(bool lumps);

  // lump extraction to files
  int ExtractLump(int item, const string &filename);
  
  // lump retrieval
  int ReadLumpHeader(int item, void *dest, int size = 0);
};


// constructor
Wad::Wad()
{
  stream = NULL;
  directory = NULL;
  diroffset = numitems = 0;
#ifdef HAVE_MD5
  memset(md5sum, 0, sizeof(md5sum))
#endif
}

Wad::~Wad()
{
  if (directory)
    free(directory);

  if (stream)
    fclose(stream);
}


void Wad::ListItems(bool lumps)
{
#ifdef HAVE_MD5
  int n = GetNumItems();
  printf(" %d lumps, MD5: ", n);
  for (int i=0; i<16; i++)
    printf("%02x:", md5sum[i]);
  printf("\n\n");
#endif

  if (!lumps)
    return;

  printf("    #  lumpname     size (B)\n"
         "----------------------------\n");
  char name8[9];
  name8[8] = '\0';

  waddir_t *p = directory;
  for (int i = 0; i < numitems; i++, p++)
  {
      strncpy(name8, p->name, 8);
      printf(" %4d  %-8s %12d\n", i, name8, p->size);
  }
}


// reads size bytes from the lump contents to the buffer dest
int Wad::ReadLumpHeader(int lump, void *dest, int size)
{
  waddir_t *l = directory + lump;

  // empty resource (usually markers like S_START, F_END ..)
  if (l->size == 0)
    return 0;
  
  // 0 size means read all the lump
  if (size == 0 || size > l->size)
    size = l->size;

  fseek(stream, l->offset, SEEK_SET);
  return fread(dest, 1, size, stream); 
}


// read a WAD file from disk
bool Wad::Open(const char *fname)
{
  stream = fopen(fname, "rb");
  if (!stream)
    return false;

  filename = fname;

  // read header
  bool fail = false;
  wadheader_t h;
  size_t result = fread(&h, sizeof(wadheader_t), 1, stream);
  if (result != 1)
    {
      printf("Could not read WAD header.\n");
      fail = true;
    }

  if (h.imagic != *reinterpret_cast<const int32_t *>("IWAD") &&
      h.imagic != *reinterpret_cast<const int32_t *>("PWAD"))
  {
      printf("Bad WAD magic number %.4s!\n", h.magic);
      fail = true;
  }

  if (fail)
    {
      fclose(stream);
      stream = NULL;
      return false;
    }
  
  // from little-endian to machine endianness
  numitems = LE_SWAP32(h.numentries);
  diroffset = LE_SWAP32(h.diroffset);

  // read wad file directory
  fseek(stream, diroffset, SEEK_SET);
  waddir_t *p = directory = (waddir_t *)malloc(numitems * sizeof(waddir_t)); 
  result = fread(directory, sizeof(waddir_t), numitems, stream);
  if (result != size_t(numitems))
    {
      printf("Could not read WAD directory.\n");
      fclose(stream);
      stream = NULL;
      return false;
    }

  // endianness conversion for directory
  for (int i = 0; i < numitems; i++, p++)
  {
      p->offset = LE_SWAP32(p->offset);
      p->size   = LE_SWAP32(p->size);
      TestPadding(p->name, 8);
  }

#ifdef HAVE_MD5
  // generate md5sum 
  rewind(stream);
  md5_stream(stream, md5sum);
#endif

  return true;
}


// create a WAD file on the disk, based on the lump lists given
int Wad::Create(const char *wadname, const vector<string> &lump_names, const vector<string> &lump_filenames, const string &lump_dir)
{
  int num_lumps = lump_names.size(); // number of lumps
  if (size_t(num_lumps) != lump_filenames.size())
    {
      printf("error: Each lump must have a name and a filename.\n");
      return -1;
    }

  bool abort = false;
  // test the lump names and try accessing files, to make this more atomic
  for (int i=0; i<num_lumps; i++)
    {
      int n = lump_names[i].length();
      if (n < 1 || n > 8)
	{
	  printf("error: lumpname %s is not acceptable.\n", lump_names[i].c_str());
	  abort = true;
	}
      
      if (lump_filenames[i] == "-")
	continue;
      
      string filename = lump_dir + lump_filenames[i];
      if (access(filename.c_str(), R_OK))
	{
	  printf("error: lump file %s cannot be accessed.\n", filename.c_str());
	  abort = true;
	}
    }
  if (abort)
    return -1;

  // construct the WAD
  FILE *outfile = fopen(wadname, "wb");
  if (!outfile)
    {
      printf("Could not create file %s.\n", wadname);
      return -1;
    }

  // WAD file layout: header, lumps, directory
  wadheader_t h;
  h.imagic = *reinterpret_cast<const int32_t *>("PWAD");
  // machine endianness to little-endian
  h.numentries = LE_SWAP32(num_lumps);
  h.diroffset = 0; // temporary, will be fixed later

  // write the header
  fwrite(&h, sizeof(wadheader_t), 1, outfile);

  vector<waddir_t> dir;
  
  // write the lumps, construct the directory
  for (int i=0; i<num_lumps; i++)
  {
    waddir_t wd;
    strncpy(wd.name, lump_names[i].c_str(), 8);
    wd.offset = ftell(outfile);

    if (lump_filenames[i] == "-")
      {
	// separator lump
	printf("empty lump: %s\n", lump_names[i].c_str());
	wd.size = 0;
      }
    else
      {
	string filename = lump_dir + lump_filenames[i];
	FILE *lumpfile = fopen(filename.c_str(), "rb");

	// get file system info about the lumpfile
	struct stat info;
	fstat(fileno(lumpfile), &info);
	int size = info.st_size;
	wd.size = size;

	// write the lump into the file
	void *buf = malloc(size);
	size_t result = fread(buf, size, 1, lumpfile);
	if (result != 1)
	  printf("Could not read lump %s from file %s.\n", lump_names[i].c_str(), filename.c_str());
	else
	  fwrite(buf, size, 1, outfile);	  
	fclose(lumpfile);
	free(buf);
      }

    dir.push_back(wd);
  }

  h.diroffset = LE_SWAP32(ftell(outfile)); // actual directory offset

  // write the directory
  for (int i=0; i<num_lumps; i++)
    {
      dir[i].offset = LE_SWAP32(dir[i].offset);
      dir[i].size   = LE_SWAP32(dir[i].size);
      fwrite(&dir[i], sizeof(waddir_t), 1, outfile);
    }

  // re-write the header with the correct diroffset
  rewind(outfile);
  fwrite(&h, sizeof(wadheader_t), 1, outfile);
  fclose(outfile);

  return num_lumps;
}


// Extracts a single lump of the WAD file into another file
int Wad::ExtractLump(int i, const string &filename)
{
  int size = GetItemSize(i);
  if (size == 0)
    return 0; // do not extract separator lumps...
     
  void *dest = malloc(size);
  ReadLumpHeader(i, dest, 0);

  FILE *output = fopen(filename.c_str(), "wb");
  if (!output)
    {
      printf("Unable to open the file %s.\n", filename.c_str());
      return -1;
    }
  fwrite(dest, size, 1, output);
  fclose(output);
  free(dest);
  return size;
}


//=============================================================================

// Print wad contents
int ListWad(const char *wadname)
{
  Wad w;

  if (!w.Open(wadname))
  {
      printf("File %s could not be opened!\n", wadname);
      return -1;
  }

  printf("WAD file %s:\n", wadname);
  w.ListItems(true);

  return 0;
}



// Create a new WAD file from the lumps listed in the given inventory file.
// Each line of the inventory file contains a filename, and the name of the lump
// that is constructed out of that file, separated by whitespace.
// If the filename is "-" (a single dash), the corresponding lump will be empty.
int CreateWad(const char *wadname, const char *inv_name, const string &lump_dir)
{
  // read the inventory file
  FILE *invfile = fopen(inv_name, "rb");
  if (!invfile)
  {
      printf("Could not open the inventory file.\n");
      return -1;
  }

  vector<string> lump_names;
  vector<string> lump_filenames;

  printf("Creating WAD file %s", wadname);
  if (lump_dir.length() > 0)
  {
      printf( " from the directory %s", lump_dir.c_str() );
  }
  printf(".\n");

  // parse the inventory file line by line
  char p[256];
  while (fgets(p, sizeof(p), invfile))
  {
      int len = strlen(p);
      int i;
      for (i=0; i<len && !isspace(p[i]); i++)
        ; // pass the lump filename
      if (i == 0)
      {
          printf("warning: you must give a filename for each lump.\n");
          continue;
      }
      p[i++] = '\0'; // pass the first ws char

      for ( ; i<len && isspace(p[i]); i++)
        ; // pass the ws

      char *lumpname = &p[i];
      for ( ; i<len && !isspace(p[i]); i++) // we're looking for a newline, but windows users will have crap like \r before it
        ; // pass the lumpname
      p[i] = '\0';

      lump_names.push_back(lumpname);
      lump_filenames.push_back(p);
  }

  fclose(invfile);
  Wad w;
  w.Create(wadname, lump_names, lump_filenames, lump_dir);
  return ListWad(wadname); // see if it opens OK
}


int ExtractWad(const char *wadname, int num, char *lumpnames[], const string &lump_dir)
{
  Wad w;
  if (!w.Open(wadname))
    {
      printf("File %s could not be opened!\n", wadname);
      return -1;
    }

  int num_lumps = w.GetNumItems();

  printf("Extracting lumps from WAD file %s", wadname);
  if (lump_dir.length() > 0)
    {
      printf( " into the directory %s", lump_dir.c_str());
    }
  printf(".\n");

  w.ListItems(false);

  // create a log file (which can be used as an inventory file when recreating the wad!)
  // C++11:  string logname = filesystem::path(wadname).filename().string() + ".log";
  const char *basename = strrchr(wadname, '/');
  if (basename)
    basename++; // skip the /
  else
    basename = wadname;
  string logname = string(basename) + ".log";
  FILE *logf = fopen(logname.c_str(), "wb");

  char name8[9];
  name8[8] = '\0';    
  int count = 0;
  int ln = 0;
  int i;

  do {
    // extract the lumps into files
    for (i = 0; i < num_lumps; i++)
      {
	const char *name = w.GetItemName(i);
	strncpy(name8, name, 8);

	if (num && strcasecmp(name8, lumpnames[ln]))
	  continue; // not the correct one

	// all extracted lumps get the .lmp extension
	string filename = lump_dir + name8 + ".lmp";
	int size = w.ExtractLump(i, filename);
	if (size >= 0)
	  {
	    printf(" %-12s: %10d bytes\n", name8, size);
	    if (size == 0)
	      fprintf(logf, "-\t\t%s\n", name8);
	    else
	      {
		count++;
		fprintf(logf, "%-12s\t\t%s\n", filename.c_str(), name8);
	      }
	  }
	
	if (num)
	  break; // extract only first instance
      }

    if (num && i == num_lumps)
      printf("Lump %s not found.\n", lumpnames[ln]);

  } while (++ln < num);

  fclose(logf);

  printf("\nDone. Wrote %d lumps.\n", count);
  return 0;
}


void help_info()
{
  printf(
	 "\nWADtool: Command line tool for manipulating WAD files.\n"
	 "Version " VERSION "\n"
	 "Copyright 2004-2020 Doom Legacy Team.\n\n"
	 "Usage:\n"
	 "  wadtool -l <wadfile>\n\tList the contents of <wadfile>.\n"
	 "  wadtool [-d <dir>] -c <wadfile> <inventoryfile>\n"
	 "\tConstruct a new <wadfile> using the given inventory file and lumps\n"
	 "\tfrom the current directory, or the given directory.\n"
	 "  wadtool [-d <dir>] -x <wadfile> [<lumpname> ...]\n"
	 "\tExtract the given lumps into the current directory (or the given directory).\n"
	 "\tIf no lumpnames are given, extract the entire contents of <wadfile>.\n"
	 );
}


int main(int argc, char *argv[])
{
  if (argc < 3 || argv[1][0] != '-')
    {
      help_info();
      return -1;
    }
  if (!strcmp(argv[1], "--help"))
    {
      help_info();
      return 0;
    }

  // Lump directory
  string lump_dir;
  
  int ret = -1;
  int ac = 1;

  // Should we use an extraction directory?
  if (argv[ac][1] == 'd')
    {
      if (argv[ac][2] == '\0')
	{
          lump_dir = string(argv[++ac]);
          ac++;
	}
      else
	{
          lump_dir = string(&argv[ac][2]);
          ac++;
	}
      char last = lump_dir.back();
      if (last != '/' && last != '\\')
        lump_dir += '/';

      const char *temp = lump_dir.c_str();
      struct stat info;

      if (stat(temp, &info) != 0)
	{
	  // directory does not exist
	  printf("Creating the directory %s.\n", temp);
	  mkdir(temp, S_IRWXU);
	}
      else if (!(info.st_mode & S_IFDIR))
	{
	  printf("%s is a not a directory.\n", temp);
	  return -1;
	}
    }

  switch (argv[ac][1])
    {
    case 'l':
      ret = ListWad(argv[ac+1]);
      break;
    case 'c':
      if (argc == ac+3)
        ret = CreateWad(argv[ac+1], argv[ac+2], lump_dir);
      else
        printf("Usage: wadtool -c wadfile.wad <inventoryfile>\n");
      break;
    case 'x':
      if (argc >= ac+3)
        ret = ExtractWad(argv[ac+1], (argc-ac-2), &argv[ac+2], lump_dir);
      else
        ret = ExtractWad(argv[ac+1], 0, NULL, lump_dir);
      break;
    default:
      printf("Unknown option '%s'\n", argv[ac]);
    }

  return ret;
}
