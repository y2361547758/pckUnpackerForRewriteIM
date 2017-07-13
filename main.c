#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#ifdef WIN32
	#include<io.h>
	#include<direct.h>
	#define md _wmkdir
#else
	#include<unistd.h>
	#include<sys/stat.h>
	#define md(fName) mkdir(fName,0777)
#endif
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
int UCS2toUTF8Code(wchar_t ucs2_code, char* utf8_code){
	 int length = 0;
	 if(!utf8_code)return 0;
	 if(0x0080 > ucs2_code){
		/* 1 byte UTF-8 Character.*/
		*utf8_code = (char)ucs2_code;
		length = 1;
	 }else if(0x0800 > ucs2_code){
		/*2 bytes UTF-8 Character.*/
		*utf8_code = ((char)(ucs2_code >> 6)) | 0xc0;
		*(utf8_code + 1) = ((char)(ucs2_code & 0x003F)) | 0x80;
		length = 2;
	 }else{
		/* 3 bytes UTF-8 Character .*/
		*utf8_code = ((char)(ucs2_code >> 12)) | 0xE0;
		*(utf8_code + 1) = ((char)((ucs2_code & 0x0FC0)>> 6)) | 0x80;
		*(utf8_code + 2) = ((char)(ucs2_code & 0x003F)) | 0x80;
		length = 3;
	 }
	 return length;
}
wchar_t* wsReadName(FILE* fN, int iCount){
	int i = 0;
	unsigned char ucBitL,ucBitH;
	wchar_t* sName = (wchar_t*)malloc((iCount/2+1)*sizeof(wchar_t));
	for(;i<iCount;i+=2){
		ucBitL = fgetc(fN);
		ucBitH = fgetc(fN);
		sName[i/2] = ucBitL | ucBitH << 8;
	}
	sName[iCount/2] = L'\0';
	return sName;
}
void vShowLog(wchar_t* wsFN, int iCount){
	int i=0,iLenFN = wcslen(wsFN),iLenT = 0;
	char sTmpFN[MAX_PATH];
	for(;i<iLenFN;++i){
		iLenT += UCS2toUTF8Code(wsFN[i], sTmpFN + iLenT);
	}
	sTmpFN[iLenT] = '\0';
	printf("*%03d:    %-48s  ...",iCount,sTmpFN);
	return;
}
int iWrite(FILE* fIn, const wchar_t* fName, unsigned long iOffset, unsigned long iFSize){
	extern int errno;
	FILE* fOut;
	#ifdef debug
		wchar_t* buffer = (wchar_t*)malloc(iFSize*sizeof(wchar_t));
	#endif
	errno = 0;
	fOut = _wfopen(fName,L"wb");
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
	wchar_t wsOutDir[MAX_PATH],*wsSubFN;
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
	iLen = strlen(argv[1]) - 4;//(wcslen(".pck") - 1);
	if(strcmp(argv[1] + iLen,".pck")){
		printf("NOT Require File!");
		return 2;
	}
	fPck = fopen(argv[1],"rb");
	if(iFileErr(fPck))return errno;
	iFSize = iGetFileSize(fPck);
	rewind(fPck);
	for(i=0;i<iLen;++i)wsOutDir[i] = argv[1][i];
	wsOutDir[iLen] = '\\';
	wsOutDir[++iLen] = '\0';
	if(!_waccess(wsOutDir, F_OK)){
		if(_waccess(wsOutDir, W_OK)){
			printf("Dir %s cannot be read",wsOutDir);
			return 3;
		}
	}else{
		md(wsOutDir);
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
		wsSubFN = wsReadName(fPck,iSubLen);
			vShowLog(wsSubFN,i+1);
		wcscpy(wsOutDir + iLen,wsSubFN);
		free(wsSubFN);
		lCurSN += iSubLen;
		//lCurSN = ftell(fPck);
		fseek(fPck,lCurSO,SEEK_SET);
		lOffset = iReadLEInt(fPck,8);
		lSubSize = iReadLEInt(fPck,8);
		lCurSO += 16;
		//lCurSO = ftell(fPck);
		if(!iWrite(fPck,wsOutDir,lOffset,lSubSize))printf("done");
		printf(".\n");
	}
	fclose(fPck);
	printf("Finished\n");
	wsOutDir[iLen] = '\0';
	wprintf(L"Pck has been unpacked in %s\n",wsOutDir);
	return 0;
}