#include "config.h"
#include "libbeye/libbeye.h"
using namespace	usr;
/**
 * @namespace	usr_tools
 * @file        beyehlp.c
 * @brief       This file is maker of BEYE help file.
 * @version     -
 * @remark      this source file is part of Binary EYE project (BEYE).
 *              The Binary EYE (BEYE) is copyright (C) 1995 Nickols_K.
 *              All rights reserved. This software is redistributable under the
 *              licence given in the file "Licence.en" ("Licence.ru" in russian
 *              translation) distributed in the BEYE archive.
 * @note        Requires POSIX compatible development system
 *
 * @author      Nickols_K
 * @since       1999
 * @note        Development, fixes and improvements
**/
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <stdexcept>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

#include "libbeye/bbio.h"
#include "libbeye/file_ini.h"
#include "libbeye/osdep/system.h"
#include "beyehelp.h"

static unsigned long items_freq = 0;
static char outfname[FILENAME_MAX];
static char id_string[80];
static char tmp_buff[0x1000];
static char o_buff[0x4000];
static char i_cache[0x1000];
static char o_cache[0x1000];
static std::ofstream ofs;

static char* archiver;

#define BBIO_CACHE_SIZE 0x1000
static const char* COMPNAME="temp_fil.tmp";
static const char* TEMPFNAME="temp_hlp.tmp";

int comp_string()
{
  unsigned i,j,slen;
  unsigned char ch;
  slen = strlen(tmp_buff);
  for(j = i = 0;i < slen;i++)
  {
     ch = tmp_buff[i];
     switch(ch)
     {
       case '%':
		{
		  i++;
		  switch(tmp_buff[i])
		  {
		     case '%': ch = '%'; break;
		     case 'B': ch = HLPC_BOLD_ON; break;
		     case 'b': ch = HLPC_BOLD_OFF; break;
		     case 'U': ch = HLPC_UNDERLINE_ON; break;
		     case 'u': ch = HLPC_UNDERLINE_OFF; break;
		     case 'I': ch = HLPC_ITALIC_ON; break;
		     case 'i': ch = HLPC_ITALIC_OFF; break;
		     case 'S': ch = HLPC_STRIKETHROUGH_ON; break;
		     case 's': ch = HLPC_STRIKETHROUGH_OFF; break;
		     case 'R': ch = HLPC_REVERSE_ON; break;
		     case 'r': ch = HLPC_REVERSE_OFF; break;
		     default:  ch = tmp_buff[i]; break;
		  }
		}
		break;
	default:break;
     }
     o_buff[j++] = ch;
  }
  o_buff[j] = 0;
  return 0;
}

void hlpCompile(const char *srcfile)
{
  FILE *in,*out;
  in = fopen(srcfile,"rt");
  if(!in) throw std::runtime_error(std::string("Can not open:")+srcfile);
  out = fopen(COMPNAME,"wb");
  if(!out) {
    fclose(in);
    throw std::runtime_error(std::string("Can not open/create:")+COMPNAME);
  }
  setvbuf(in,i_cache,_IOFBF,sizeof(i_cache));
  setvbuf(out,o_cache,_IOFBF,sizeof(o_cache));
  while(1)
  {
     if(!fgets(tmp_buff,sizeof(tmp_buff),in))
     {
       if(feof(in)) break;
       fclose(in);
       fclose(out);
       throw std::runtime_error(std::string("Can not read from: ")+srcfile);
     }
     comp_string();
     if(fputs(o_buff,out) == EOF)
     {
       fclose(in);
       fclose(out);
       throw std::runtime_error(std::string("Can not write to: ")+COMPNAME);
     }
  }
  fclose(in);
  fclose(out);
}

bool __FASTCALL__ MyCallBack(const IniInfo& ini,any_t* data)
{
  UNUSED(data);
  if(strcmp(ini.section,"ITEMS") == 0) {
     if(strcmp(ini.subsection,"") == 0) {
       items_freq++;
     }
  }
  if(strcmp(ini.section,"OPTION") == 0) {
     if(strcmp(ini.subsection,"") == 0) {
       if(strcmp(ini.item,"output") == 0) {
	 strncpy(outfname,ini.value,sizeof(outfname));
	 outfname[sizeof(outfname)-1] = 0;
       }
       if(strcmp(ini.item,"id") == 0) {
	 strncpy(id_string,ini.value,sizeof(id_string));
	 id_string[sizeof(id_string)-1] = 0;
       }
     }
  }
  return false;
}

bool __FASTCALL__ MyCallOut(const IniInfo& ini,any_t* data)
{
  UNUSED(data);
  if(strcmp(ini.section,"ITEMS") == 0)
  {
     if(strcmp(ini.subsection,"") == 0)
     {
	unsigned long litem,fpos;
	unsigned copysize;
	beye_help_item bhi;
	binary_stream* bIn;
	int handle;
	fpos = ofs.tellp();
	printf("Processing: %s\n",ini.value);
	std::istringstream is(ini.item);
	is>>litem;
	sprintf(bhi.item_id,"%08lX",litem);
	hlpCompile(ini.value);
	strcpy(tmp_buff,archiver);
	strcat(tmp_buff," e ");
	strcat(tmp_buff,COMPNAME);
	strcat(tmp_buff," ");
	strcat(tmp_buff,TEMPFNAME);
	if(system(tmp_buff)) throw std::runtime_error(std::string("Error ")+strerror(errno)+" ocurred while processing: "+tmp_buff);
	bIn = new binary_stream;
	bool rc;
	rc = bIn->open(TEMPFNAME,binary_stream::FO_READONLY | binary_stream::SO_DENYNONE);
	if(rc == false)
	rc = bIn->open(TEMPFNAME,binary_stream::FO_READONLY | binary_stream::SO_COMPAT);
	if(rc == false) throw std::runtime_error(std::string("Can not open ")+TEMPFNAME);
	litem = bIn->flength();
	sprintf(bhi.item_length,"%08lX",litem);
	handle = ::open(COMPNAME,binary_stream::FO_READONLY | binary_stream::SO_DENYNONE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if(handle == -1) throw std::runtime_error(std::string("Can not open ")+ini.value);
	litem = ::lseek(handle,0L,SEEK_END);
	::close(handle);
	__filesize_t flength,fpos2;
	fpos2=ofs.tellp();
	ofs.seekp(0L,std::ios_base::end);
	flength=ofs.tellp();
	ofs.seekp(fpos2,std::ios_base::beg);
	sprintf(bhi.item_decomp_size,"%08lX",litem);
	sprintf(bhi.item_off,"%08lX",flength);
	ofs.seekp(items_freq*sizeof(beye_help_item)+strlen(id_string)+1+HLP_SLONG_LEN,std::ios_base::beg);
	ofs.write((const char*)&bhi,sizeof(beye_help_item));
	ofs.seekp(fpos,std::ios_base::beg);
	litem = bIn->flength();
	do
	{
	   copysize = std::min((unsigned long)0x1000,litem);
	   binary_packet bp=bIn->read(copysize); memcpy(tmp_buff,bp.data(),bp.size());
	   ofs<<bp;
	   litem -= copysize;
	} while(litem);
	delete bIn;
	ofs.flush();
	items_freq++;
     }
  }
  return false;
}

static void my_atexit() { }
char **ArgVector;

int main( int argc, char *argv[] )
{
  beye_help_item bhi;
  unsigned long i;
  char sout[HLP_SLONG_LEN];
  if(argc < 3)
  {
     printf("Wrong number of arguments\n\
	     Usage: beyehlp archiver project.file\n");
     return -1;
  }
  strcpy(id_string,BEYE_HELP_VER);
  ArgVector=argv;
  atexit(my_atexit);
  System msystem();
  archiver=argv[1];
  Ini_Parser::scan(argv[2],MyCallBack,NULL);
  printf("Using %s as archiver\n"
	"Using %s as project\n"
	"Using %s as output filename\n"
	, argv[1]
	, argv[2]
	, outfname);
  memset(&bhi,0,sizeof(beye_help_item));
  if(binary_stream::exists(outfname)) if(binary_stream::unlink(outfname)) { fprintf(stderr,"Can not delete %s\n",argv[2]); return -1; }
  ofs.open(outfname,std::ios_base::binary);
    if(!ofs.is_open()) {
	fprintf(stderr,"Can not work with %s",outfname);
	return -1;
    }
  ofs.write(id_string,strlen(id_string)+1);
  sprintf(sout,"%08lX",items_freq);
  ofs.write((const char*)sout,HLP_SLONG_LEN);
  for(i = 0;i < items_freq;i++) ofs.write((const char*)&bhi,sizeof(beye_help_item));
  items_freq = 0;
  Ini_Parser::scan(argv[2],MyCallOut,NULL);
  ofs.close();
  binary_stream::unlink(TEMPFNAME);
  binary_stream::unlink(COMPNAME);
  printf("Help file looks - ok\n");
  return 0;
}
