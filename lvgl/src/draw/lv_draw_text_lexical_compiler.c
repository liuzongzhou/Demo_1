/**
 * @file lv_draw_text_lexical_compiler.c
 *
 */


/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>
#include <stddef.h>
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