/**
 * @file lv_draw_text_lexical_compiler.c
 *
 */


/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>


#include "lv_draw_text_lexical_compiler.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

//lvgl\src\draw\lv_draw_text_lexical_compiler.c

void textarea_init(){
    char path1[MAX_LINE_LENGTH];
    char path2[MAX_LINE_LENGTH];
    char path3[MAX_LINE_LENGTH];

    char current_dir[MAX_LINE_LENGTH];
    //获取当前工作目录
    _getcwd(current_dir,MAX_LINE_LENGTH);
    printf("Current directory: %s\n", current_dir);
    // 构建文件路径
    snprintf(path1, sizeof(path1), "%s%s%s", current_dir, "/", "controlKeyWord.ini");
    snprintf(path2, sizeof(path2), "%s%s%s", current_dir, "/", "macroKeyWord.ini");
    snprintf(path3, sizeof(path3), "%s%s%s", current_dir, "/", "operatorkeyWord.ini");

    // 加载关键词
    if (load_keywords(path1, &controlKeywordsSet) &&
        load_keywords(path2, &macroProgramKeywordsSet) &&
        load_keywords(path3, &operatorKeywordsSet)) {
        printf("success\n");
    } else {
        printf("error\n");
    }
}

// 初始化集合
void init_set(KeywordSet *s) {
    s->count = 0;
}

// 添加元素到集合
bool add_keyword(KeywordSet *ks, const char *keyword) {
    if (ks->count >= MAX_KEYWORDS) {
        return false; 
    }
    ks->keywords[ks->count] = strdup(keyword);
    //printf("Keyword : %s\n", ks->keywords[ks->count]);
    if (ks->keywords[ks->count] == NULL) {
        return false; // 分配内存失败
    }
    ks->count++;
    return true;
}

// 检查集合是否包含指定元素
bool contains(const KeywordSet *s, const char *keyword) {
    printf("inputKeyword : %s\n", keyword);
    for (int i = 0; i < s->count; i++) {
        printf("Keyword : %s\n", s->keywords[i]);
        if (strcmp(s->keywords[i], keyword) == 0) {
            return true;
        }
    }
    return false;
}

// 正则表达式匹配
bool regex_match(const char *pattern, const char *str) {
    // // regex_t reg;
    // // int ret = regcomp(&reg, pattern, REG_EXTENDED | REG_ICASE);
    // // if (ret != 0) {
    // //     fprintf(stderr, "Regex compilation failed: %s\n", regerror(ret, &reg, NULL, 0));
    // //     regfree(&reg);
    // //     return false;
    // // }

    // // ret = regexec(&reg, str, 0, NULL, 0);
    // // regfree(&reg);
    // return ret == 0;
    return false;
}

// 从文件中加载关键词
bool load_keywords(const char *filename, KeywordSet *keywords) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return false;
    }

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        // 移除行尾的换行符
        line[strcspn(line, "\n")] = '\0';

        // 添加关键词到集合
        if (!add_keyword(keywords, line)) {
            // 如果添加失败，释放已经分配的内存并返回 false
            for (int i = 0; i < keywords->count; i++) {
                free(keywords->keywords[i]);
            }
            keywords->count = 0;
            fclose(file);
            return false;
        }
    }

    fclose(file);
    return true;
}

// 设置从 chPtr-1 开始向前 stylePtr 个位置的值
void set_styling(int *setStyling, int chPtr, int stylePtr, int value) {
    // 计算起始位置
    int start = chPtr - 1 - stylePtr + 1;

    // 确保起始位置不会小于 0
    if (start < 0) {
        start = 0;
    }

    // 设置指定范围内的值
    for (int i = start; i <= chPtr; i++) {
        setStyling[i] = value;
    }
}


// 检查字符是否为字母
bool isLetter(char ch) {
    if ((ch >= 'A') && (ch <= 'Z')) {
        return true;
    } else {
        return false;
    }
}

// 检查字符是否为关键词字母
bool isKeyLetter(char ch) {
    // 将小写字母转换为大写字母
    if (ch >= 'a') ch -= 32;

    // 定义关键词字符串
    const char *key_letters = "GMSTFN";

    // 检查字符是否包含在关键词字符串中
    return strchr(key_letters, ch) != NULL;
}

// 检查字符是否为数字
bool isDigit(char ch) {
    if (ch >= '0' && ch <= '9')
        return true;
    else if (ch == '.')
        return true;
    return false;
}

/* Function to check if a character is an operator*/
bool isOperator(char ch) {
    const char *operators = "+-*><=&|!,^";
    size_t length = strlen(operators);

    // Iterate through the operators string to find the character
    for (size_t i = 0; i < length; i++) {
        if (operators[i] == ch) {
            return true;
        }
    }

    // If not found, return false
    return false;
}







/**********************
 *   STATIC FUNCTIONS
 **********************/