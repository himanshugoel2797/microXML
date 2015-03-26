#define _CRT_SECURE_NO_WARNINGS
#define STACK_SIZE 512		//512 character stack
#define SPLIT_SIZE 512		//Stream in blocks of approx 512 bytes
#define MAX_TAG_LENGTH 64

#include <stdio.h>
#include <stdlib.h>

typedef struct {
	void(*error)();
	void(*openTag)(char*, int);
	void(*closeTag)(char*, int);
	void(*contentHit)(char*, int);
	void(*attributeHit)(char*, int, char*, int);
} _eventHandlers;
_eventHandlers* handlers;
char *stringStack;
char *strA, *strB;
int stringStackLen = 0;
int stringStackPos = 0;
int maxStringLength = 0;


void error()
{
	printf("Error\n");
}

void openTag(char* tagName, int len)
{
	printf("OPEN TAG: %.*s\n", len, tagName);
}

void closeTag(char* tagName, int len)
{
	printf("CLOSE TAG: %.*s\n", len, tagName);
}

void contentHit(char* content, int len)
{
	printf("\tCONTENT: %.*s\n", len, content);
}

void attributeHit(char* attrName, int attrName_len, char* attrVal, int attrVal_len)
{
	printf("ATTRIBUTE NAME: %.*s \nATTRIBUTE VALUE: %.*s\n", attrName_len, attrName, attrVal_len, attrVal);
}

void pushStack(char* val, int len)
{
	if (len > maxStringLength)maxStringLength = len;
	for (int i = stringStackPos; i < len + stringStackPos; i++)stringStack[i] = val[i - stringStackPos];
	stringStackPos += len;
	stringStack[stringStackPos++] = 0;
}
char* popStack()
{
	int i = stringStackPos - 2;
	for (; i > 0 && stringStack[i]; i--){
		strA[stringStackPos - i - 2] = stringStack[i];
	}
	i = stringStackPos - i - 2;
	for (int j = 0; j < i; j++){
		strB[j] = strA[i - j - 1];
	}
	strB[i] = 0;
	stringStackPos -= ++i;
	return strB;
}
int strcmpA(char* a, char* b, int len){
	for (int i = 0; i < len; i++){
		if (a[i] != b[i])return 0;
	}
	return 1;
}

int addToStack(char* piece, int len){
	int tag_name_len;
	int attribute_hit = -1;
	for (tag_name_len = 0; piece[tag_name_len] != '>' && tag_name_len < len; tag_name_len++)
	{
		if (piece[tag_name_len] == ' '){
			attribute_hit = 0;
			break;
		}
	}
	if (piece[0] - '/'){
		handlers->openTag(piece, tag_name_len);
		pushStack(piece, tag_name_len);
	}
	else{
		handlers->closeTag(piece + 1, tag_name_len - 1);
		if (!strcmpA(popStack(), piece + 1, tag_name_len - 1))handlers->error();
	}
	int end_pos = tag_name_len;
	if (!attribute_hit){
		int attribute_name_pos = 0;
		int attribute_name_len = 0;
		int is_in_attr = 1;
		for (attribute_hit = 0; attribute_hit + tag_name_len < len; attribute_hit++){
			if (piece[attribute_hit + tag_name_len] == '=')attribute_name_len = attribute_hit - attribute_name_pos;
			if ((piece[attribute_hit + tag_name_len] == ' ' || piece[attribute_hit + tag_name_len] == '>') && is_in_attr == 3){
				handlers->attributeHit(piece + attribute_name_pos + tag_name_len, attribute_name_len, piece + tag_name_len + attribute_name_pos + attribute_name_len + 1, attribute_hit - attribute_name_len - 1 - attribute_name_pos);
				is_in_attr = 1;
				if (piece[attribute_hit + tag_name_len] == '>'){
					end_pos = attribute_hit + tag_name_len;
					break;
				}
				if (piece[attribute_hit + tag_name_len] == ' ')attribute_name_pos = attribute_hit;
			}
			if (piece[attribute_hit + tag_name_len] == '"')is_in_attr++;
		}
	}
	return end_pos;
}

int checkRangeForWhitespace(char* str, int min, int max){
	for (int i = min; i < max; i++)if (str[i] - ' ' && str[i] - '\n' && str[i] - '\r' && str[i] - '\t')return 0;
	return 1;
}

void initParser(){
	stringStackLen = STACK_SIZE;
	stringStack = malloc(stringStackLen);
	strA = malloc(MAX_TAG_LENGTH);
	strB = malloc(MAX_TAG_LENGTH);
}

void destroyParser(){
	free(strA);
	free(strB);
	free(stringStack);
	free(handlers);
}

void xmlParse(char* xml, int len)
{
	int prevTag = 0;
	for (int i = 0; i < len; i++){
		if (xml[i] == '<'){
			if (i > prevTag && !checkRangeForWhitespace(xml, prevTag, i))handlers->contentHit(xml + prevTag, i - prevTag);
			i += addToStack(xml + i + 1, len - i - 1);
			prevTag = i + 2;
		}
	}
}

int main(int argc, char** args){

	handlers = malloc(sizeof(_eventHandlers));
	handlers->attributeHit = &attributeHit;
	handlers->closeTag = &closeTag;
	handlers->error = &error;
	handlers->openTag = &openTag;
	handlers->contentHit = &contentHit;

	FILE* file = fopen("xml.txt", "r");
	fseek(file, 0, SEEK_END);
	int length = ftell(file);
	int mallocedLength = length;
	char* xmlFile;
	initParser();
	int num = length / SPLIT_SIZE;
	fseek(file, 0, SEEK_SET);
	for (int x = 0; x <= (num + 1); x++){
		if (num > 1)mallocedLength = SPLIT_SIZE;
		xmlFile = malloc(mallocedLength);
		fread(xmlFile, 1, mallocedLength, file);
		for (int i = mallocedLength - 1; i >= 0; i--){
			if (xmlFile[i] == '>')break;
			else {
				fseek(file, -1, SEEK_CUR);
				xmlFile[i] = 0;
				mallocedLength--;
			}
		}
		xmlParse(xmlFile, mallocedLength);
		free(xmlFile);
	}
	fclose(file);
	destroyParser();

	getchar();
}