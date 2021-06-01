#include "stdio.h"
#include "stdlib.h"
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
#define hex32(a,b,c,d) (a|(b<<8)|(c<<16)|(d<<24))
char* tabs = "								";
struct savetrak{
u32 size;
u32 type;

int mvhd_offs;
int mvhd_size;
//
int mdhd_offs;
int mdhd_size;
//
int stsd_offs;
int stsd_size;
//
int ctts_offs;
int ctts_size;
int stts_offs;
int stts_size;
//
int stsc_offs;
int stsc_size;
//
int stsz_offs;
int stsz_size;
//
int stco_offs;
int stco_size;
int co64_offs;
int co64_size;
};
static struct savetrak trackdef[16];
static int trackcnt = 0;




u16 swap16(u16 in)
{
	return ((in<<8)&0xff00) | ((in>>8)&0xff);
}
u32 swap32(u32 in)
{
	return (in>>24) | ((in>>8)&0xff00) | ((in<<8)&0xff0000) | (in<<24);
}
u64 swap64(u64 in)
{
	u64 lo = in&0xffffffff;
	u64 hi = (in>>32)&0xffffffff;
	lo = swap32(lo);
	hi = swap32(hi);
	return (lo<<32) | hi;
}
void print8(void* buf, int len)
{
	u8* p = buf;
	printf("%x,%x,%x,%x,%x,%x,%x,%x\n",p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
}




u64 parse_mdat(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	printf("%.*smdat\n",depth,tabs);
	return 0;
}




struct mvhd{
	u32 size;
	u32 mvhd;
	u8 ver;
	u8 flag[3];
	u32 time_create;
	u32 time_modify;
	u32 time_scale;
	u32 time_duration;
	u16 speed[2];
	u8 volume[2];
	u8 rsvd[10];
	u8 matrix[36];
	u32 preview_time;
	u32 preview_duration;
	u32 poster_time;
	u32 selection_time;
	u32 selection_duration;
	u32 curr_time;
	u32 next_trackID;
}__attribute__((packed));
int parse_mvhd(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct mvhd* m = (void*)(p[depth-1]);
	int end = off + swap32(m->size);
	u32 scale = swap32(m->time_scale);
	u32 duration = swap32(m->time_duration);

	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*stime_create=%x\n",depth,tabs,
		swap32(m->time_create));
	printf("%.*stime_modify=%x\n",depth,tabs,
		swap32(m->time_modify));
	printf("%.*stime_scale=%x\n",depth,tabs,
		scale);
	printf("%.*stime_duration=%x(totaltime=%f)\n",depth,tabs,
		duration, (float)duration/(float)scale);
	printf("%.*sspeed=%x.%x\n",depth,tabs,
		swap16(m->speed[0]), swap16(m->speed[1]));
	printf("%.*svolume=%x.%x\n",depth,tabs,
		m->volume[0], m->volume[1]);
	printf("%.*spreview_time=%x\n",depth,tabs,
		swap32(m->preview_time));
	printf("%.*spreview_duration=%x\n",depth,tabs,
		swap32(m->preview_duration));
	printf("%.*sposter_time=%x\n",depth,tabs,
		swap32(m->poster_time));
	printf("%.*sselection_time=%x\n",depth,tabs,
		swap32(m->selection_time));
	printf("%.*sselection_duration=%x\n",depth,tabs,
		swap32(m->selection_duration));
	printf("%.*scurr_time=%x\n",depth,tabs,
		swap32(m->curr_time));
	printf("%.*snext_trackID=%x\n",depth,tabs,
		swap32(m->next_trackID));

	trackdef[trackcnt].mvhd_offs = off;
	trackdef[trackcnt].mvhd_size = end-off;
	return 0;
}
struct mfhd{
	u32 size;
	u32 mvhd;
	u8 ver;
	u8 flag[3];
	u32 seq;
}__attribute__((packed));
int parse_mfhd(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct mfhd* m = (void*)(p[depth-1]);
	int end = off + swap32(m->size);

	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*sseq=%x\n",depth,tabs,
		swap32(m->seq));
}




struct stsd{	//sample description
	u32 size;
	u32 stsd;
	u8 ver;
	u8 flag[3];
	u32 desc_count;
	u8 tmp[];
}__attribute__((packed));
int parse_stsd(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct stsd* m = (void*)(p[depth-1]);
	int end = off + swap32(m->size);
	int count = swap32(m->desc_count);
	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*sdesc_count=%x\n",depth,tabs,
		count);

	int j=off+16;
	u32 k=0;
	u8* buf = m->tmp;
	for(;count>0;count--){
		k = (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+buf[3];
		printf("%.*s[%x,%x)=%.4s\n",depth+1,tabs, j,j+k,buf+4);

		if(k<0)break;
		j += k;
		buf += k;
		if(j >= end)break;
	}

	trackdef[trackcnt].stsd_offs = off;
	trackdef[trackcnt].stsd_size = end-off;
	return 0;
}




struct ctts{	//pts to dts table
	u32 size;
	u32 ctts;
	u8 ver;
	u8 flag[3];
	u32 time_to_sample_count;
	u8 tmp[];
}__attribute__((packed));
int parse_ctts(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct ctts* m = (void*)(p[depth-1]);
	int end = off + swap32(m->size);
	int count = swap32(m->time_to_sample_count);
	printf("%.*sver=%x\n",depth,tabs, m->ver);
};




struct stts_inner{
	u32 count;
	u32 duration;
}__attribute__((packed));
struct stts{	//time-sample	//index table
	u32 size;
	u32 stts;
	u8 ver;
	u8 flag[3];
	u32 time_to_sample_count;
	u8 tmp[];
}__attribute__((packed));
int parse_stts(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct stts* m = (void*)(p[depth-1]);
	int end = off + swap32(m->size);
	int count = swap32(m->time_to_sample_count);
	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*sdesc_count=%x\n",depth,tabs,
		count);

	int j=off+16;
	u32 k=0;
	u8* buf = m->tmp;
	for(;count>0;count--){
		printf("%.*s[%x,%x):count=%x,duration=%x\n",depth+1,tabs, j,j+8,
			swap32(*(u32*)buf), swap32(*(u32*)(buf+4)));

		j += 8;
		buf += 8;
		if(j >= end)break;
	}

	trackdef[trackcnt].stts_offs = off;
	trackdef[trackcnt].stts_size = end-off;
	return 0;
}
int parse_stts_get_sample(struct stts* stts, int t)
{
	struct stts_inner* in = (void*)stts->tmp;
	int max = swap32(stts->time_to_sample_count);
	int k,curr,next;
	int count,duration;

	curr = 0;
	for(k=0;k<max;k++){
		count = swap32(in[k].count);
		duration = swap32(in[k].duration);
		next = curr + count * duration;
		if(t < curr)return -1;
		if(t >= next){
			curr = next;
			continue;
		}

		//printf("t=%d,curr=%d,duration=%d\n",t,curr,duration);
		return 1 + (t-curr) / duration;
	}
	return -2;
}
struct stss{	//sync sample
	u32 size;
	u32 stss;
	u8 ver;
	u8 flag[3];
	u32 sync_sample_count;
	u8 tmp[];
}__attribute__((packed));
int parse_stss(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct stss* m = (void*)(p[depth-1]);
	int end = off + swap32(m->size);
	int count = swap32(m->sync_sample_count);
	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*sdesc_count=%x\n",depth,tabs,
		count);

	int j=off+16;
	u32 k=0;
	u8* buf = m->tmp;
	for(;count>0;count--){
		printf("%.*s[%x,%x):kframe=%x\n",depth+1,tabs, j,j+4,
			swap32(*(u32*)buf) );

		j += 4;
		buf += 4;
		if(j >= end)break;
	}
	return 0;
}
struct stsc_inner{
	u32 first_chunk;
	u32 sample_per_chunk;
	u32 sample_desc_id;
}__attribute__((packed));
struct stsc{	//sample-chunk	//index table
	u32 size;
	u32 stsc;
	u8 ver;
	u8 flag[3];
	u32 sample_to_chunk_count;
	u8 tmp[];
		//u32 first chunk
		//u32 sample per chunk
		//u32 sample description ID
}__attribute__((packed));
int parse_stsc(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct stsc* m = (void*)(p[depth-1]);
	int end = off + swap32(m->size);
	int count = swap32(m->sample_to_chunk_count);
	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*sdesc_count=%x\n",depth,tabs,
		count);

	int j=off+16;
	u32 k=0;
	u8* buf = m->tmp;
	for(;count>0;count--){
		printf("%.*s[%x,%x):1st=%x,per=%x,id=%x\n",depth+1,tabs, j,j+12,
		swap32(*(u32*)buf), swap32(*(u32*)(buf+4)), swap32(*(u32*)(buf+8)) );

		j += 12;
		buf += 12;
		if(j >= end)break;
	}

	trackdef[trackcnt].stsc_offs = off;
	trackdef[trackcnt].stsc_size = end-off;
	return 0;
}
int parse_stsc_get_chunk(struct stsc* stsc, int sample, int* chunk, int* this_chunk_first_sample)
{
	struct stsc_inner* in = (void*)stsc->tmp;
	int max = swap32(stsc->sample_to_chunk_count);
	int k,curr,next;
	int firstchunk, endchunk, sampleperchunk, sampledescid;

	curr = 1;
	for(k=0;k<max;k++){
		firstchunk = swap32(in[k].first_chunk);
		if(k+1 == max)endchunk = firstchunk+1;
		else endchunk = swap32(in[k+1].first_chunk);

		sampleperchunk = swap32(in[k].sample_per_chunk);
		sampledescid = swap32(in[k].sample_desc_id);

		next = curr + sampleperchunk*(endchunk-firstchunk);
		if(sample < curr)return -1;
		if(sample >= next){
			curr = next;
			continue;
		}

		*chunk = firstchunk + (sample-curr) / sampleperchunk;
		*this_chunk_first_sample = curr + sampleperchunk * ((*chunk)-firstchunk);
		return 0;
	}

	return -2;
}
struct stsz{	//sample size
	u32 size;
	u32 stsz;
	u8 ver;
	u8 flag[3];
	u32 sample_size;
	u32 sample_count;
	u32 eachsize[];
}__attribute__((packed));
int parse_stsz(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct stsz* m = (void*)(p[depth-1]);
	int end = off + swap32(m->size);
	int count = swap32(m->sample_count);
	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*ssample_size=%x\n",depth,tabs,
		swap32(m->sample_size));
	printf("%.*ssample_count=%x\n",depth,tabs,
		count);

	u32* buf = (void*)p[depth];
	int ret;

	int j=off+20;	//file offset
	u32 k;		//0 to count
	u32 at = 0;
	u32 sz = 0;
	for(k=0;k<count;k++){
		if(0 == (k%0x400)){
			fseek(fp, j, SEEK_SET);
			//if(ret < 0)

			ret = fread(buf, 1, 0x1000, fp);
			if(ret <= 0)break;
		}

		sz = swap32(buf[k%0x400]);
		printf("%.*s[%x,%x):id=%x,at=%x,sz=%x\n",depth+1,tabs, j,j+4,
			k+1, at, sz);
		at += sz;

		j += 4;
		if(j >= end)break;
	}

	trackdef[trackcnt].stsz_offs = off;
	trackdef[trackcnt].stsz_size = end-off;
	return 0;
}
int parse_stsz_get_sampleinchunkoff(struct stsz* stsz, int sample, int this_chunk_first_sample, int* sample_size)
{
	int k;
	int offs=0;
	for(k=this_chunk_first_sample-1;k<sample-1;k++){
		offs += swap32(stsz->eachsize[k]);
		//printf("k=%x,off=%x\n",k,offs);
	}

	*sample_size = swap32(stsz->eachsize[sample-1]);
	return offs;
}
struct stco{	//chunk offset
	u32 size;
	u32 stco;
	u8 ver;
	u8 flag[3];
	u32 chunk_count;
	u32 offs[];
}__attribute__((packed));
int parse_stco(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct stco* m = (void*)(p[depth-1]);
	int end = off + swap32(m->size);
	int count = swap32(m->chunk_count);
	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*sdesc_count=%x\n",depth,tabs,
		count);

	int j=off+16;
	u32 k=0;
	for(k=0;k<count;k++){
		printf("%.*s[%x,%x):offs=%x\n",depth+1,tabs, j,j+4,
		swap32(m->offs[k]) );

		j += 4;
		if(j >= end)break;
	}

	trackdef[trackcnt].stco_offs = off;
	trackdef[trackcnt].stco_size = end-off;
	return 0;
}
int parse_stco_get_chunkinfileoff(struct stco* stco, int chunk)
{
	int max = swap32(stco->size);
	if(chunk > max)return -1;
	return swap32(stco->offs[chunk-1]);
}
struct co64{	//chunk offset
	u32 size;
	u32 co64;
	u8 ver;
	u8 flag[3];
	u32 chunk_count;
	u64 offs[];
}__attribute__((packed));
int parse_co64(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct co64* m = (void*)(p[depth-1]);
	int end = off + swap32(m->size);
	int count = swap32(m->chunk_count);
	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*sdesc_count=%x\n",depth,tabs,
		count);

	int j=off+16;
	u32 k=0;
	for(k=0;k<count;k++){
		printf("%.*s[%x,%x):offs=%llx\n",depth+1,tabs, j,j+4,
		swap64(m->offs[k]) );

		j += 4;
		if(j >= end)break;
	}

	trackdef[trackcnt].co64_offs = off;
	trackdef[trackcnt].co64_size = end-off;
	return 0;
}
int parse_co64_get_chunkinfileoff(struct co64* co64, int chunk)
{
	int max = swap32(co64->size);
	if(chunk > max)return -1;
	return swap64(co64->offs[chunk-1]);
}
int parse_stbl(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	//printf("%.*strak\n",depth,tabs);
	unsigned char* pre = p[depth-1];
	int end = off + swap32(*(u32*)pre);

	int j=off+8;
	int k=0;
	unsigned char* buf = p[depth];
	for(;;){
		fseek(fp, j, SEEK_SET);

		int ret = fread(buf, 1, 0x1000, fp);
		if(ret <= 0)return 0;

		k = (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+buf[3];
		printf("%.*s[%x,%x)=%.4s\n",depth,tabs, j,j+k,buf+4);

		switch(*(unsigned int*)(buf+4)){
		case hex32('s','t','s','d'):
			parse_stsd(fp, j, p, depth+1);
			break;
		case hex32('s','t','s','s'):
			parse_stss(fp, j, p, depth+1);
			break;
		case hex32('c','t','t','s'):
			parse_ctts(fp, j, p, depth+1);
			break;
		case hex32('s','t','t','s'):
			parse_stts(fp, j, p, depth+1);
			break;
		case hex32('s','t','s','c'):
			parse_stsc(fp, j, p, depth+1);
			break;
		case hex32('s','t','s','z'):
			parse_stsz(fp, j, p, depth+1);
			break;
		case hex32('s','t','c','o'):
			parse_stco(fp, j, p, depth+1);
			break;
		case hex32('c','o','6','4'):
			parse_co64(fp, j, p, depth+1);
			break;
		}

		if(k<0)break;
		j += k;
		if(j >= end)break;
	}
	return 0;
}




struct dref{
	u32 size;
	u32 dref;
	u8 ver;
	u8 flag[3];
	u32 entry_count;
}__attribute__((packed));
int parse_dref(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct dref* m = (void*)(p[depth-1]);

	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*sentry_count=%x\n",depth,tabs,
		swap32(m->entry_count));
	return 0;
}
int parse_dinf(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	unsigned char* pre = p[depth-1];
	int end = off + swap32(*(u32*)pre);

	int j=off+8;
	int k=0;
	unsigned char* buf = p[depth];
	for(;;){
		fseek(fp, j, SEEK_SET);

		int ret = fread(buf, 1, 0x1000, fp);
		if(ret <= 0)return 0;

		k = (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+buf[3];
		printf("%.*s[%x,%x)=%.4s\n",depth,tabs, j,j+k,buf+4);

		switch(*(unsigned int*)(buf+4)){
		case hex32('d','r','e','f'):
			parse_dref(fp, j, p, depth+1);
			break;
		}

		if(k<0)break;
		j += k;
		if(j >= end)break;
	}
	return 0;
}




struct vmhd{
	u32 size;
	u32 vmhd;
	u8 ver;
	u8 flag[3];
	u32 graphics_mode;
	u16 opcolor[3];
}__attribute__((packed));
int parse_vmhd(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	//printf("%.*s????\n",depth,tabs);
	struct vmhd* m = (void*)(p[depth-1]);

	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*sgraphics_mode=%x\n",depth,tabs,
		swap32(m->graphics_mode));
	printf("%.*sopcode:r=%x,g=%x,b=%x\n",depth,tabs,
		swap16(m->opcolor[0]), swap16(m->opcolor[1]), swap16(m->opcolor[2]));
	return 0;
}
struct smhd{
	u32 size;
	u32 smhd;
	u8 ver;
	u8 flag[3];
	u8 balance[2];
}__attribute__((packed));
int parse_smhd(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	//printf("%.*s????\n",depth,tabs);
	struct smhd* m = (void*)(p[depth-1]);

	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*sgraphics_mode=%x.%x\n",depth,tabs,
		m->balance[0], m->balance[1]);
	return 0;
}
int parse_minf(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	//printf("%.*strak\n",depth,tabs);
	unsigned char* pre = p[depth-1];
	int end = off + swap32(*(u32*)pre);

	int j=off+8;
	int k=0;
	unsigned char* buf = p[depth];
	for(;;){
		fseek(fp, j, SEEK_SET);

		int ret = fread(buf, 1, 0x1000, fp);
		if(ret <= 0)return 0;

		k = (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+buf[3];
		printf("%.*s[%x,%x)=%.4s\n",depth,tabs, j,j+k,buf+4);

		switch(*(unsigned int*)(buf+4)){
		case hex32('v','m','h','d'):
			parse_vmhd(fp, j, p, depth+1);
			break;
		case hex32('s','m','h','d'):
			parse_smhd(fp, j, p, depth+1);
			break;
		case hex32('d','i','n','f'):
			parse_dinf(fp, j, p, depth+1);
			break;
		case hex32('s','t','b','l'):
			parse_stbl(fp, j, p, depth+1);
			break;
		}

		if(k<0)break;
		j += k;
		if(j >= end)break;
	}
	return 0;
}




struct mdhd{
	u32 size;
	u32 mdhd;
	u8 ver;
	u8 flag[3];
	u32 time_create;
	u32 time_modify;
	u32 time_scale;
	u32 time_duration;
	u16 language;
	u16 predefined;
}__attribute__((packed));
int parse_mdhd(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct mdhd* m = (void*)(p[depth-1]);
	int end = off + swap32(m->size);
	u32 scale = swap32(m->time_scale);
	u32 duration = swap32(m->time_duration);

	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*stime_create=%x\n",depth,tabs,
		swap32(m->time_create));
	printf("%.*stime_modify=%x\n",depth,tabs,
		swap32(m->time_modify));
	printf("%.*stime_scale=%x\n",depth,tabs,
		scale);
	printf("%.*stime_duration=%x(totaltime=%f)\n",depth,tabs,
		duration, (float)duration/(float)scale);
	printf("%.*slanguage=%x\n",depth,tabs,
		swap16(m->language));

	trackdef[trackcnt].mdhd_offs = off;
	trackdef[trackcnt].mdhd_size = end-off;
	return 0;
}
struct hdlr{
	u32 size;
	u32 hdlr;
	u8 ver;
	u8 flag[3];
	u32 predefined;
	u32 handlertype;
		//vide = video track
		//soun = audio track
		//hint = hint track
	u8 rsvd[12];
	char name[0];
		//"VideoHandler"
}__attribute__((packed));
int parse_hdlr(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct hdlr* m = (void*)(p[depth-1]);
	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*spredefined=%x\n",depth,tabs,
		swap32(m->predefined));
	printf("%.*shandlertype=%.4s\n",depth,tabs,
		(char*)&m->handlertype);
	printf("%.*sname=%s\n",depth,tabs,
		m->name);

	trackdef[trackcnt].type = m->handlertype;
	return 0;
}
int parse_mdia(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	//printf("%.*smdia\n",depth,tabs);
	unsigned char* pre = p[depth-1];
	int end = off + swap32(*(u32*)pre);

	int j=off+8;
	int k=0;
	unsigned char* buf = p[depth];
	for(;;){
		fseek(fp, j, SEEK_SET);

		int ret = fread(buf, 1, 0x1000, fp);
		if(ret <= 0)return 0;

		k = (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+buf[3];
		printf("%.*s[%x,%x)=%.4s\n",depth,tabs, j,j+k,buf+4);

		switch(*(unsigned int*)(buf+4)){
		case hex32('m','d','h','d'):
			parse_mdhd(fp, j, p, depth+1);
			break;
		case hex32('h','d','l','r'):
			parse_hdlr(fp, j, p, depth+1);
			break;
		case hex32('m','i','n','f'):
			parse_minf(fp, j, p, depth+1);
			break;
		}

		if(k<0)break;
		j += k;
		if(j >= end)break;
	}
	return 0;
}




struct tkhd{
	u32 size;
	u32 tkhd;
	u8 ver;
	u8 flag[3];
	u32 time_create;
	u32 time_modify;
	u32 trackid;
	u32 rsvd;
	u32 duration;
	u32 rsvd00;
	u32 rsvd01;
	u16 layer;
	u16 altgroup;
	u8 volume[2];
	u16 rsvd3;
	u8 matrix[36];
	u16 width[2];
	u16 height[2];
}__attribute__((packed));
int parse_tkhd(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct tkhd* m = (void*)(p[depth-1]);

	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*stime_create=%x\n",depth,tabs,
		swap32(m->time_create));
	printf("%.*stime_modify=%x\n",depth,tabs,
		swap32(m->time_modify));
	printf("%.*strackid=%x\n",depth,tabs,
		swap32(m->trackid));
	printf("%.*sduration=%x\n",depth,tabs,
		swap32(m->duration));
	printf("%.*slayer=%x\n",depth,tabs,
		swap16(m->layer));
	printf("%.*saltgroup=%x\n",depth,tabs,
		swap16(m->altgroup));
	printf("%.*svolume=%x.%x\n",depth,tabs,
		m->volume[0], m->volume[1]);
	printf("%.*swidth=%d.%d\n",depth,tabs,
		swap16(m->width[0]), swap16(m->width[1]) );
	printf("%.*sheight=%d.%d\n",depth,tabs,
		swap16(m->height[0]), swap16(m->height[1]) );
	return 0;
}
struct tfhd{
	u32 size;
	u32 tfhd;
	u8 ver;
	u8 flag[3];
	u32 trackid;
}__attribute__((packed));
int parse_tfhd(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct tfhd* m = (void*)(p[depth-1]);

	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*strackid=%x\n",depth,tabs,
		swap32(m->trackid));
	return 0;
}
struct tfdt{
	u32 size;
	u32 tfhd;
	u8 ver;
	u8 flag[3];
	union{
		u32 decodetime32;
		u64 decodetime64;
	};
}__attribute__((packed));
int parse_tfdt(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct tfdt* m = (void*)(p[depth-1]);
	int ver = m->ver;
	printf("%.*sver=%x\n",depth,tabs, ver);
	printf("%.*sdecodetime=%llx\n",depth,tabs,
		(1==ver)?swap64(m->decodetime64):swap32(m->decodetime32) );
	return 0;
}
struct trun{
	u32 size;
	u32 tfhd;
	u8 ver;
	u8 flag[3];
	u32 samplecount;
	u8 haha[0];
}__attribute__((packed));
int parse_trun(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct trun* m = (void*)(p[depth-1]);
	u32 flag = (m->flag[0]<<16) | (m->flag[1]<<8) | (m->flag[2]);
	u32 samplecount = swap32(m->samplecount);
	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*sflag=%x\n",depth,tabs, flag);
	printf("%.*ssamplecount=%x\n",depth,tabs, samplecount);

	int j;
	u8* tmp = m->haha;
	if(flag&1){	//have_data_offset
		printf("%.*sdataoffset=%x(from moof)\n",depth,tabs,
			swap32(*(u32*)tmp) );
		tmp += 4;
	}
	if(flag&4){	//have_first_sample_flag
		printf("%.*s1stflag=%x\n",depth,tabs,
			swap32(*(u32*)tmp) );
		tmp += 4;
	}

	u32 at = 0;
	u32 nt = 0;
	u32 sample_duration;
	u32 sample_size;
	u32 sample_flag;
	u32 sample_ct;
	for(j=0;j<samplecount;j++){
		if(flag&0x100){	//non-default duration
			sample_duration = swap32(*(u32*)tmp);
			tmp += 4;
		}else sample_duration = 0;
		if(flag&0x200){	//non-default size
			sample_size = swap32(*(u32*)tmp);
			nt = at + sample_size;
			tmp += 4;
		}else sample_size = 0;
		if(flag&0x400){	//non-default flag
			sample_flag = swap32(*(u32*)tmp);
			tmp += 4;
		}else sample_flag = 0;
		if(flag&0x800){	//non-default ct
			sample_ct = swap32(*(u32*)tmp);
			tmp += 4;
		}else sample_ct = 0;

		printf("%.*s%x:[%x,%x),du=%x,fl=%x,ct=%x\n",depth+1,tabs,
			j, at, nt, sample_duration, sample_flag, sample_ct);
		at = nt;
	}
	return 0;
}
struct trex{
	u32 size;
	u32 trex;
	u8 ver;
	u8 flag[3];
	u32 defindex;
	u32 defduration;
	u32 defsize;
	u32 defflag;
}__attribute__((packed));
int parse_trex(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	struct trex* m = (void*)(p[depth-1]);

	printf("%.*sver=%x\n",depth,tabs, m->ver);
	printf("%.*sindex=%x\n",depth,tabs,
		swap32(m->defindex));
	printf("%.*sduration=%x\n",depth,tabs,
		swap32(m->defduration));
	printf("%.*ssize=%x\n",depth,tabs,
		swap32(m->defsize));
	printf("%.*sflag=%x\n",depth,tabs,
		swap32(m->defflag));
	return 0;
}




int parse_edts(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	return 0;
}




int parse_trak(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	//printf("%.*strak\n",depth,tabs);
	unsigned char* pre = p[depth-1];
	int end = off + swap32(*(u32*)pre);

	trackdef[trackcnt].type = 0;

	int j=off+8;
	int k=0;
	unsigned char* buf = p[depth];
	for(;;){
		fseek(fp, j, SEEK_SET);

		int ret = fread(buf, 1, 0x1000, fp);
		if(ret <= 0)break;

		k = (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+buf[3];
		printf("%.*s[%x,%x)=%.4s\n",depth,tabs, j,j+k,buf+4);

		switch(*(unsigned int*)(buf+4)){
		case hex32('t','k','h','d'):
			parse_tkhd(fp, j, p, depth+1);
			break;
		case hex32('e','d','t','s'):
			parse_edts(fp, j, p, depth+1);
			break;
		case hex32('m','d','i','a'):
			parse_mdia(fp, j, p, depth+1);
			break;
		}

		if(k<0)break;
		j += k;
		if(j >= end)break;
	}

	trackcnt += 1;
	return 0;
}
int parse_traf(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	//printf("%.*straf\n",depth,tabs);
	unsigned char* pre = p[depth-1];
	int end = off + swap32(*(u32*)pre);

	int j=off+8;
	int k=0;
	unsigned char* buf = p[depth];
	for(;;){
		fseek(fp, j, SEEK_SET);

		int ret = fread(buf, 1, 0x1000, fp);
		if(ret <= 0)break;

		k = (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+buf[3];
		printf("%.*s[%x,%x)=%.4s\n",depth,tabs, j,j+k,buf+4);

		switch(*(unsigned int*)(buf+4)){
		case hex32('t','f','h','d'):
			parse_tfhd(fp, j, p, depth+1);
			break;
		case hex32('t','f','d','t'):
			parse_tfdt(fp, j, p, depth+1);
			break;
		case hex32('t','r','u','n'):
			parse_trun(fp, j, p, depth+1);
			break;
		}

		if(k<0)break;
		j += k;
		if(j >= end)break;
	}

	return 0;
}
int parse_mvex(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	printf("%.*smvex\n",depth,tabs);
	unsigned char* pre = p[depth-1];
	int end = off + swap32(*(u32*)pre);

	int j=off+8;
	int k=0;
	unsigned char* buf = p[depth];
	for(;;){
		fseek(fp, j, SEEK_SET);

		int ret = fread(buf, 1, 0x1000, fp);
		if(ret <= 0)break;

		k = (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+buf[3];
		printf("%.*s[%x,%x)=%.4s\n",depth,tabs, j,j+k,buf+4);

		switch(*(unsigned int*)(buf+4)){
		case hex32('t','r','e','x'):
			parse_trex(fp, j, p, depth+1);
			break;
		}

		if(k<0)break;
		j += k;
		if(j >= end)break;
	}
	return 0;
}
int parse_udta(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	printf("%.*sudta\n",depth,tabs);
	return 0;
}
int parse_moov(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	//printf("%.*smoov\n",depth,tabs);
	unsigned char* pre = p[depth-1];
	int end = off + swap32(*(u32*)pre);

	int j=off+8;
	int k=0;
	unsigned char* buf = p[depth];
	for(;;){
		fseek(fp, j, SEEK_SET);

		int ret = fread(buf, 1, 0x1000, fp);
		if(ret <= 0)return 0;

		k = (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+buf[3];
		printf("%.*s[%x,%x)=%.4s\n",depth,tabs, j,j+k,buf+4);

		switch(*(unsigned int*)(buf+4)){
		case hex32('m','v','h','d'):
			parse_mvhd(fp, j, p, depth+1);
			break;
		case hex32('t','r','a','k'):
			parse_trak(fp, j, p, depth+1);
			break;
		case hex32('u','d','t','a'):
			parse_udta(fp, j, p, depth+1);
			break;
		case hex32('m','v','e','x'):
			parse_mvex(fp, j, p, depth+1);
			break;
		}

		if(k<0)break;
		j += k;
		if(j >= end)break;
	}
	return 0;
}
int parse_moof(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	//printf("%.*smoov\n",depth,tabs);
	unsigned char* pre = p[depth-1];
	int end = off + swap32(*(u32*)pre);

	int j=off+8;
	int k=0;
	unsigned char* buf = p[depth];
	for(;;){
		fseek(fp, j, SEEK_SET);

		int ret = fread(buf, 1, 0x1000, fp);
		if(ret <= 0)return 0;

		k = (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+buf[3];
		printf("%.*s[%x,%x)=%.4s\n",depth,tabs, j,j+k,buf+4);

		switch(*(unsigned int*)(buf+4)){
		case hex32('m','f','h','d'):
			parse_mfhd(fp, j, p, depth+1);
			break;
		case hex32('t','r','a','f'):
			parse_traf(fp, j, p, depth+1);
			break;
		}

		if(k<0)break;
		j += k;
		if(j >= end)break;
	}
	return 0;
}
int parse_ftyp(FILE* fp,int off, unsigned char (*p)[0x1000],int depth)
{
	unsigned char* buf = p[0];
	printf("%.*smajor=%.4s\n",depth,tabs, buf+8);
	printf("%.*sminor=%x\n",depth,tabs, buf[15]);
	printf("%.*scapatible=%.4s,%.4s,%.4s\n",depth,tabs,
		buf+16,
		buf+20,
		buf+24
	);
	return 0;
}




int parse(FILE* fp, unsigned char (*p)[0x1000], int depth)
{
	//printf("%x,%x,%x,%x\n",p[4],p[5],p[6],p[7]);
	int j=0;
	u64 k = 0;

	unsigned char* buf = p[depth];
	for(;;){
		fseek(fp, j, SEEK_SET);

		int ret = fread(buf, 1, 0x1000, fp);
		if(ret <= 0)return 0;

		k = (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+buf[3];
		if(k <= 1){
			k = (buf[8]<<24)+(buf[9]<<16)+(buf[10]<<8)+buf[11];
			k = k<<32;
			k += (buf[12]<<24)+(buf[13]<<16)+(buf[14]<<8)+buf[15];
		}
		printf("[%x,%llx)=%.4s\n",j,j+k,buf+4);

		switch(*(unsigned int*)(buf+4)){
		case hex32('f','t','y','p'):
			parse_ftyp(fp, j, p, depth+1);
			break;
		case hex32('m','d','a','t'):
			parse_mdat(fp, j, p, depth+1);
			break;
		case hex32('m','o','o','v'):
			parse_moov(fp, j, p, depth+1);
			break;
		case hex32('m','o','o','f'):
			parse_moof(fp, j, p, depth+1);
			break;
		}

		if(k<0)break;
		j += k;
	}
	printf("-------------------\n");
	return 0;
}




#define MVHD 0
#define MDHD 1
#define CTTS 2
#define STTS 3
#define STSC 4
#define STSZ 5
#define STCO 6
#define CO64 7
int getsample(FILE* fp, u8 (*tmp)[0x1000], float pts, int* sample_sz)
{
	int ret;

	struct mvhd* mvhd = (void*)tmp[MVHD];
	int mvhd_offs = trackdef[0].mvhd_offs;
	int mvhd_size = trackdef[0].mvhd_size;
	printf("[%x,%x]mvhd\n", mvhd_offs, mvhd_size);
	ret = fseek(fp, mvhd_offs, SEEK_SET);
	ret = fread(mvhd, 1, 0x1000, fp);
	//print8(mvhd, 8);

	struct mdhd* mdhd = (void*)tmp[MDHD];
	int mdhd_offs = trackdef[0].mdhd_offs;
	int mdhd_size = trackdef[0].mdhd_size;
	printf("[%x,%x]mdhd\n", mdhd_offs, mdhd_size);
	ret = fseek(fp, mdhd_offs, SEEK_SET);
	ret = fread(mdhd, 1, 0x1000, fp);
	//print8(mdhd, 8);

	struct ctts* ctts = (void*)tmp[CTTS];
	int ctts_offs = trackdef[0].ctts_offs;
	int ctts_size = trackdef[0].ctts_size;
	printf("[%x,%x]ctts\n", ctts_offs, ctts_size);
	ret = fseek(fp, ctts_offs, SEEK_SET);
	ret = fread(ctts, 1, 0x1000, fp);
	//print8(ctts, 8);

	struct stts* stts = (void*)tmp[STTS];
	int stts_offs = trackdef[0].stts_offs;
	int stts_size = trackdef[0].stts_size;
	printf("[%x,%x]stts\n", stts_offs, stts_size);
	ret = fseek(fp, stts_offs, SEEK_SET);
	ret = fread(stts, 1, 0x1000, fp);
	//print8(stts, 8);

	struct stsc* stsc = (void*)(tmp[STSC]);
	int stsc_offs = trackdef[0].stsc_offs;
	int stsc_size = trackdef[0].stsc_size;
	printf("[%x,%x]stsc\n", stsc_offs, stsc_size);
	ret = fseek(fp, stsc_offs, SEEK_SET);
	ret = fread(stsc, 1, 0x1000, fp);
	//print8(stsc, 8);

	struct stco* stco = (void*)(tmp[STCO]);
	int stco_offs = trackdef[0].stco_offs;
	int stco_size = trackdef[0].stco_size;
	printf("[%x,%x]stco\n", stco_offs, stco_size);
	ret = fseek(fp, stco_offs, SEEK_SET);
	ret = fread(stco, 1, 0x1000, fp);
	//print8(stco, 8);

	struct stsz* stsz = (void*)(tmp[STSZ]);
	int stsz_offs = trackdef[0].stsz_offs;
	int stsz_size = trackdef[0].stsz_size;
	printf("[%x,%x]stsz\n", stsz_offs, stsz_size);
	ret = fseek(fp, stsz_offs, SEEK_SET);
	ret = fread(stsz, 1, 0x1000, fp);
	//print8(stsz, 8);

	//0.pts -> dts
	float dts = pts;	//todo:ctts

	//1.dts -> mp4 time
	int scale = swap32(mdhd->time_scale);
	int duration = swap32(mdhd->time_duration);
	float total = (float)duration / scale;
	if((dts < 0.0) | (dts > total)){
		printf("err1: must between[0,%f)\n", total);
		return 0;
	}
	int mp4time = dts * scale;
	printf("pts=%f,dts=%f,mp4time=%d\n",pts,dts,mp4time);

	//2.(stts)mp4 time -> sample number
	int sample = parse_stts_get_sample(stts, mp4time);
	if(sample < 0){
		printf("err2: time to sample wrong\n");
		return 0;
	}
	printf("sample=%d(decimal count from 1)\n",sample);

	//3.(stsc)sample number -> chunk number
	int this_chunk_first_sample = -1;
	int chunknum = -1;
	parse_stsc_get_chunk(stsc, sample, &chunknum, &this_chunk_first_sample);
	if(chunknum < 0){
		printf("err3: sample to chunknum wrong\n");
		return 0;
	}
	printf("chunknum=%d, this_chunk_first_sample=%d(decimal count from 1)\n",
		chunknum, this_chunk_first_sample);

	//4.(stsz)sample_number -> sample_in_chunk
	int sample_size = 0;
	int sample_in_chunk = parse_stsz_get_sampleinchunkoff(
		stsz, sample, this_chunk_first_sample, &sample_size);
	if(sample_in_chunk < 0){
		printf("err4: sample to sample_in_chunk wrong\n");
		return 0;
	}
	printf("sample_in_chunk=%x, sample_size=%x(hexadecimal count from 0)\n",
		sample_in_chunk, sample_size);

	//5.(stco)chunk number -> chunk_in_file
	int chunk_in_file = parse_stco_get_chunkinfileoff(stco, chunknum);
	if(chunk_in_file < 0){
		printf("err5: chunk to chunk_in_file wrong\n");
		return 0;
	}
	printf("chunk_in_file=%x(hexadecimal count from 0)\n", chunk_in_file);

	//6.sample_in_chunk + chunk_in_file = sample offset
	int sample_in_file = sample_in_chunk + chunk_in_file;
	printf("sample_in_file=%x(hexadecimal count from 0)\n", sample_in_file);

	printf("-------------------\n");
	*sample_sz = sample_size;
	return sample_in_file;
}




void codec_mp4v(u8* buf)
{
	switch(buf[3]){
	case 0xb0:
		printf("visual_object_sequence_start_code\n");
		break;
	case 0xb1:
		printf("visual_object_sequence_end_code\n");
		break;
	case 0xb2:
		printf("user_data_start_code\n");
		break;
	case 0xb3:
		printf("group_of_vop_start_code\n");
		break;
	case 0xb4:
		printf("video_session_error_code\n");
		break;
	case 0xb5:
		printf("visual_object_start_code\n");
		break;
	case 0xb6:
		printf("vop_start_code\n");
		if(0x00 == buf[4])printf("Iframe\n");
		if(0x01 == buf[4])printf("Pframe\n");
		if(0x02 == buf[4])printf("Bframe\n");
		break;
	case 0xb7:
		printf("slice_start_code\n");
		break;
	case 0xb8:
		printf("extension_start_code\n");
		break;
	case 0xc3:
		printf("stuffing_start_code\n");
		break;
	}
}
void codec_mp4a(u8* buf)
{
	printf("codec_mp4a\n");
}
void codec_avc1(u8* buf)
{
	printf("@codec_avc1\n");
}
void codec_vp9(u8* buf)
{
	printf("@codec_vp9\n");
}
void codec_hevc(u8* buf)
{
	printf("@codec_hevc\n");
}
void codec_av1(u8* buf)
{
	printf("@codec_av1\n");
}
int codec(FILE* fp, int off, u8* buf, int len)
{
	int ret;
	u32 fmt;
	struct stsd* stsd = (void*)buf;
	int stsd_offs = trackdef[0].stsd_offs;
	int stsd_size = trackdef[0].stsd_size;
	printf("[%x,%x]stsd\n", stsd_offs, stsd_size);
	ret = fseek(fp, stsd_offs, SEEK_SET);
	ret = fread(stsd, 1, 0x1000, fp);
	print8(stsd, 8);

	printf("%.4s\n", stsd->tmp+4);
	fmt = *(u32*)(stsd->tmp+4);


	ret = fseek(fp, off, SEEK_SET);
	ret = fread(buf, 1, len<0x10000?len:0x10000, fp);
	print8(buf, 8);

	switch(fmt){
	case hex32('m','p','4','v'):
		codec_mp4v(buf);
		break;
	case hex32('m','p','4','a'):
		codec_mp4a(buf);
		break;
	case hex32('a','v','c','1'):
		codec_avc1(buf);
		break;
	case hex32('h','e','v','1'):
	case hex32('h','v','c','1'):
		codec_hevc(buf);
		break;
	case hex32('v','p','0','9'):
		codec_vp9(buf);
		break;
	case hex32('a','v','0','1'):
		codec_av1(buf);
		break;
	default:
		printf("codec_unknown\n");
	}
	printf("-------------------\n");
	return 0;
}




int main(int argc, char** argv)
{
	unsigned char tmp[16][0x1000];
	if(argc < 2){
		printf("./a.out /path/file.mp4 3.14\n");
		return 0;
	}

	float pts = 0.0;
	if(3 == argc){
		pts = atof(argv[2]);
		printf("pts=%f\n", pts);
	}

	FILE* fp = fopen(argv[1],"rb");
	if(!fp)return 0;

	int ret = parse(fp, tmp, 0);

	int j;
	printf("--------------------------------\n");
	for(j=0;j<trackcnt;j++){
		printf("trak[%d]=%.4s\n", j, (char*)&trackdef[j].type);
	}
/*
	int sample_size;
	int sample_in_file;
	for(;;){
 	      	sample_in_file = getsample(fp, tmp, pts, &sample_size);
		if(sample_in_file>0)codec(fp, sample_in_file, (void*)tmp, sample_size);

		if(scanf("%f", &pts) <= 0)break;
	}
*/
	fclose(fp);
	return 0;
}
