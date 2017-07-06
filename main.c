#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#ifdef WIN32
	#include<io.h>
	#include<direct.h>
	#define md _mkdir
#else
	#include<unistd.h>
	#include<sys/stat.h>
	#define md(fName) mkdir(fName,0777)
#endif
typedef unsigned short UCS2;
typedef unsigned char UINT8;
size_t iGetFileSize(FILE *fn){
	size_t ret=-1;
	if(fn!=NULL){
		fseek(fn,0,SEEK_END);
		ret = ftell(fn);
	}
	return ret;
}
int iFileErr(FILE* fN){
	extern int errno;
	if(!fN || ferror(fN)){
		switch(errno){
			case 13: printf("Permission Deny!");break;
			case 2: printf("File Not Found!");break;
			default: printf("Unknow error code %d",errno);
		}
		clearerr(fN);
		return errno;
	}
	return 0;
}
unsigned long long iReadLEInt(FILE* fN, int iCount){
	unsigned char cBit;
	int i = 0;
	unsigned long long iInt = 0;
	for(;i<iCount;++i){
		cBit = fgetc(fN);
		iInt |= cBit << i*8;
	}
	return iInt;
}
UINT8 UCS2toUTF8Code(UCS2 ucs2_code, UINT8* utf8_code){
	 int length = 0;
	 if(!utf8_code)return 0;
	 if(0x0080 > ucs2_code){
		/* 1 byte UTF-8 Character.*/
		*utf8_code = (UINT8)ucs2_code;
		length = 1;
	 }else if(0x0800 > ucs2_code){
		/*2 bytes UTF-8 Character.*/
		*utf8_code = ((UINT8)(ucs2_code >> 6)) | 0xc0;
		*(utf8_code + 1) = ((UINT8)(ucs2_code & 0x003F)) | 0x80;
		length = 2;
	 }else{
		/* 3 bytes UTF-8 Character .*/
		*utf8_code = ((UINT8)(ucs2_code >> 12)) | 0xE0;
		*(utf8_code + 1) = ((UINT8)((ucs2_code & 0x0FC0)>> 6)) | 0x80;
		*(utf8_code + 2) = ((UINT8)(ucs2_code & 0x003F)) | 0x80;
		length = 3;
	 }
	 return length;
}
char* sReadName(FILE* fN, int iCount){
	int i = 0,iLen = 0;
	unsigned char ucBitL,ucBitH;
	UCS2 ucChar;
	//char* sName = (char*)malloc((iCount/2*3+1)*sizeof(char));
	char* sName = (char*)malloc(MAX_PATH);
	for(;i<iCount;i+=2){
		ucBitL = fgetc(fN);
		ucBitH = fgetc(fN);
		ucChar = ucBitL | ucBitH << 8;
		iLen += UCS2toUTF8Code(ucChar, sName + iLen);
	}
	sName[iLen] = '\0';
	return sName;
}
int iWrite(FILE* fIn, const char* fName, unsigned long iOffset, unsigned long iFSize){
	extern int errno;
	FILE* fOut;
	#ifdef debug
		char* buffer = (char*)malloc(iFSize*sizeof(char));
	#endif
	errno = 0;
	fOut = fopen(fName,"wb");
	if(iFileErr(fOut))return errno;
	fseek(fIn,iOffset,SEEK_SET);
	#ifdef debug
		//Load sub-file to memory
		fread(buffer,sizeof(char),iFSize,fIn);
		fwrite(buffer,sizeof(char),iFSize,fOut);
		free(buffer);
	#else
		//violence
		while(iFSize--/* && !ferror(fIn) && !ferror(fOut)*/)fputc(fgetc(fIn),fOut);
	#endif
	fclose(fOut);
	return errno;
}
//#define debug
#ifdef debug
const int argc = 2;
const char *argv[] = {"mail.c.exe","D:\\Programing\\Git\\pckUnpackerForRewriteIM\\__mov.pck"};
int main() {
#else
int main(int argc, const char *argv[]) {
#endif
	extern int errno;
	FILE *fPck,*fOut;
	size_t iLen,iFSize;
	unsigned char sOutDir[MAX_PATH],*sSubFN;
	unsigned int iL1,iCount,iW,iH,iSubLen,i;
	unsigned long lSubSize,lOffset,lCurSL,lCurSN,lCurSO;
	
	//Open and check the pck file
	if(argc<2){
		printf("Usage:\n");
		printf("\t%s [pck File]\n",argv[0]);
		printf("\tNote: please use full path\n");
		return 0;
	}
	if(access(argv[1], F_OK)){
		printf("Cannot Open this File");
		return 1;
	}
	iLen = strlen(argv[1]) - 4;//(strlen(".pck") - 1);
	if(strcmp(argv[1] + iLen,".pck")){
		printf("NOT Require File!");
		return 2;
	}
	fPck = fopen(argv[1],"rb");
	if(iFileErr(fPck))return errno;
	iFSize = iGetFileSize(fPck);
	rewind(fPck);

	//Make the output dir
	strncpy(sOutDir,argv[1],iLen);
	sOutDir[iLen] = '\\';
	sOutDir[++iLen] = '\0';
	if(!access(sOutDir, F_OK)){
		if(access(sOutDir, W_OK)){
			printf("Dir %s cannot be read",sOutDir);
			return 3;
		}
	}else{
		md(sOutDir);
	}

	//Read and divide the pck
	iL1 = iReadLEInt(fPck,4);
	if(iL1 != 1){
		printf("Unknow type");
		return 4;
	}
	iCount = iReadLEInt(fPck,4);
	printf("%d files\n",iCount);
	iW = iReadLEInt(fPck,4);
	iH = iReadLEInt(fPck,4);
	printf("width:%d heigh:%d (unknow data)\n",iW,iH);
	iReadLEInt(fPck,4*4);	//16bytes pad
	lCurSL = ftell(fPck);
	lCurSN = lCurSL + iCount*4;
	for(i=iSubLen=0;i<iCount;++i)iSubLen += iReadLEInt(fPck,4);
	//lCurSN = ftell(fPck);
	lCurSO = lCurSN + iSubLen;
	while(lCurSO % 4)++lCurSO;
	for(i=0;i<iCount;++i){
		fseek(fPck,lCurSL,SEEK_SET);
		iSubLen = iReadLEInt(fPck,4);
		lCurSL += 4;
		//lCurSL = ftell(fPck);
		fseek(fPck,lCurSN,SEEK_SET);
		sSubFN = sReadName(fPck,iSubLen);
			printf("*%03d:    %-48s  ...",i+1,sSubFN);
		strcpy(sOutDir + iLen,sSubFN);
		free(sSubFN);
		lCurSN += iSubLen;
		//lCurSN = ftell(fPck);
		fseek(fPck,lCurSO,SEEK_SET);
		lOffset = iReadLEInt(fPck,8);
		lSubSize = iReadLEInt(fPck,8);
		lCurSO += 16;
		//lCurSO = ftell(fPck);
		if(!iWrite(fPck,sOutDir,lOffset,lSubSize))printf("done");
		printf(".\n");
	}
	fclose(fPck);
	printf("Finished\n");
	sOutDir[iLen] = '\0';
	printf("Pck was been unpacked in %s\n",sOutDir);
	return 0;
}