/**
 * @file lv_draw_text_lexical_compiler.h
 *
 */

#ifndef LV_DRAW_TEXT_LEXICAL_COMPILER_H
#define LV_DRAW_TEXT_LEXICAL_COMPILER_H

#define MAX_KEYWORDS 100
#define MAX_LINE_LENGTH 100

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/


/*********************
 *      DEFINES
 *********************/


/**********************
 *      TYPEDEFS
 **********************/
enum {
        NORMAL = 0,
        CONTROLKEYWORD = 1,
        COMMENT = 2,
        MACROVAR = 3,
        OPERATOR = 4,
        CHANNEL = 5,
        NROMALLETTTER = 6,
        MACROPROGRAM = 7,
        G = 8,
        M = 9,
        S = 10,
        T = 11,
        N = 12,
        F = 13,
        ERROR = 14,
        MACROVAREX = 15,
    };



// 定义一个集合结构体
typedef struct {
    char *keywords[MAX_KEYWORDS]; // 关键词数组
    int count;                    // 关键词数量
} KeywordSet;

KeywordSet operatorKeywordsSet;
KeywordSet controlKeywordsSet;
KeywordSet macroProgramKeywordsSet;
/**********************
 * GLOBAL PROTOTYPES
 **********************/
void textarea_init();
void set_styling(int *setStyling, int chPtr, int stylePtr, int value);
bool isOperator(char ch);
bool isLetter(char ch);
bool isKeyLetter(char ch);
bool isDigit(char ch);


void init_set(KeywordSet *s);
bool add_keyword(KeywordSet *ks, const char *keyword);
bool contains(const KeywordSet *s, const char *keyword);
bool load_keywords(const char *filename, KeywordSet *keywords);
/***********************
 * GLOBAL VARIABLES
 ***********************/

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_DRAW_TEXT_LEXICAL_COMPILER_H*/
