%option noyywrap
%{
/*****************声明和选项设置  begin*****************/
#include <stdio.h>
#include <stdlib.h>

#include "lexical_analyzer.h"

int lines;
int pos_start;
int pos_end;

/*****************声明和选项设置  end*****************/

%}
COMMENT \/\*(?:[^\*]|\*+[^\/\*])*\*+\/
BLANK ([ ]|\t)+
EOL \n
LETTER [a-zA-Z]
ID [a-zA-Z]+
INTEGER [0-9]+
FLOAT ([0-9]+\.|[0-9]*\.[0-9]+)
ARRAY \[\]

%%

 /******************TODO*********************/
 /****请在此补全所有flex的模式与动作  start******/
 //STUDENT TO DO
{COMMENT} {
           return COMMENT;
           }
{BLANK} {
        pos_start = pos_end;
        pos_end += strlen(yytext);
        return BLANK;
        }
{EOL}   {
        lines++;
		pos_start = 1;
		pos_end = 1;
        return EOL;
        }
"+"     {pos_start = pos_end;
        pos_end += strlen(yytext);
        return ADD;
        }
"-"    {pos_start = pos_end;
        pos_end += strlen(yytext);
        return SUB;
        }
"*"     {pos_start = pos_end;
        pos_end += strlen(yytext);
        return MUL;
        }
"/"     {pos_start = pos_end;
        pos_end += 1;
        return DIV;
        }
"<"    {pos_start = pos_end;
        pos_end += strlen(yytext);
        return LT;
        }
"<="    {pos_start = pos_end;
        pos_end += strlen(yytext);
        return LTE;
        }
">"     {pos_start = pos_end;
        pos_end += strlen(yytext);
        return GT;
        }
">="    {pos_start = pos_end;
        pos_end += strlen(yytext);
        return GTE;
        }
"=="    {pos_start = pos_end;
        pos_end += strlen(yytext);
        return EQ;
        }
"!="    {pos_start = pos_end;
        pos_end += strlen(yytext);
        return NEQ;
        }
"="       {pos_start = pos_end;
        pos_end += strlen(yytext);
        return ASSIN;
        }
";"       {pos_start = pos_end;
        pos_end += strlen(yytext);
        return SEMICOLON;
        }
","       {pos_start = pos_end;
        pos_end += strlen(yytext);
        return COMMA;
        }
"("      {pos_start = pos_end;
        pos_end += strlen(yytext);
        return LPARENTHESE;
        }
")"      {pos_start = pos_end;
        pos_end += strlen(yytext);
        return RPARENTHESE;
        }
"["      {pos_start = pos_end;
        pos_end += strlen(yytext);
        return LBRACKET;
        }
"]"      {pos_start = pos_end;
        pos_end += strlen(yytext);
        return RBRACKET;
        }
"{"      {pos_start = pos_end;
        pos_end += strlen(yytext);
        return LBRACE;
        }
"}"      {pos_start = pos_end;
        pos_end += strlen(yytext);
        return RBRACE;
        }
else    {pos_start = pos_end;
        pos_end += strlen(yytext);
        return ELSE;
        }
if      {pos_start = pos_end;
        pos_end += strlen(yytext);
        return IF;
        }
int     {pos_start = pos_end;
        pos_end += strlen(yytext);
        return INT;
        }
float   {pos_start = pos_end;
        pos_end += strlen(yytext);
        return FLOAT;
        }
return  {pos_start = pos_end;
        pos_end += strlen(yytext);
        return RETURN;
        }
void    {pos_start = pos_end;
        pos_end += strlen(yytext);
        return VOID;
        }
while   {pos_start = pos_end;
        pos_end += strlen(yytext);
        return WHILE;
        }
{ID}      {pos_start = pos_end;
        pos_end += strlen(yytext);
        return IDENTIFIER;
        }
{INTEGER} {pos_start = pos_end;
        pos_end += strlen(yytext);
        return INTEGER;
        }
{FLOAT}   {pos_start = pos_end;
        pos_end += strlen(yytext);
        return FLOATPOINT;
        }
{ARRAY}   {pos_start = pos_end;
        pos_end += strlen(yytext);
        return ARRAY;
        }
. {pos_start = pos_end;
        pos_end += 1;
	return ERROR;}





 /****请在此补全所有flex的模式与动作  end******/
%%
/****************C代码 start*************/

/// \brief analysize a *.cminus file
///
/// \param input_file, 需要分析的文件路径
/// \param token stream, Token_Node结构体数组，用于存储分析结果，具体定义参考lexical_analyer.h
int find_sub_string(const char* str1,const char* str2)
{

    if(!strstr(str1,str2)) return 0;

    int num = 0;
    while(*str1)
    {
        if(strstr(str1,str2) != NULL)
            num++;      //判定为子字符串，次数加一
        else    break;
        str1 = strstr(str1,str2)+1;
 /*strstr返回子字符串开始位置，然后位置后移一位，重新判断*/
        for(*str1; *str1 !='\0'; str1++)
        {
            if(*str1 == *str2)
                break;
        }
    }
    return num;
}
void analyzer(char* input_file, Token_Node* token_stream){
    lines = 1;
    pos_start = 1;
    pos_end = 1;
	int i = 1;
    if(!(yyin = fopen(input_file,"r"))){
        printf("[ERR] No input file\n");
        exit(1);
    }
    printf("[START]: Read from: %s\n", input_file);

    int token;
    int index = 0;

    while(token = yylex()){
        switch(token){
            case COMMENT:
                if(!find_sub_string(yytext,"\n")){
                    pos_end += strlen(yytext);
                }
				else {
					lines += find_sub_string(yytext,"\n");
					for(i = yyleng; yytext[i] != '\n'; i--);
					pos_end = yyleng - i;
					pos_start = pos_end;
				}
            case BLANK:
                //STUDENT TO DO
                break;
            case EOL:
                //STUDENT TO DO
                break;
            case ERROR:
                printf("[ERR]: unable to analysize %s at %d line, from %d to %d\n", yytext, lines, pos_start, pos_end);
            default :
                if (token == ERROR){
                    sprintf(token_stream[index].text, "[ERR]: unable to analysize %s at %d line, from %d to %d", yytext, lines, pos_start, pos_end);
                } else {
                    strcpy(token_stream[index].text, yytext);
                }
                token_stream[index].token = token;
                token_stream[index].lines = lines;
                token_stream[index].pos_start = pos_start;
                token_stream[index].pos_end = pos_end;
                index++;
                if (index >= MAX_NUM_TOKEN_NODE){
                    printf("%s has too many tokens (> %d)", input_file, MAX_NUM_TOKEN_NODE);
                    exit(1);
                }
        }
    }
    printf("[END]: Analysis completed.\n");
    return;
}



/****************C代码 end*************/
