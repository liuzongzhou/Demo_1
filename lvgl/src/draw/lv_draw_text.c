/**
 * @file lv_draw_text.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw.h"
#include "lv_draw_text.h"
#include "lv_draw_text_lexical_compiler.h"
#include "../misc/lv_math.h"
#include "../hal/lv_hal_disp.h"
#include "../core/lv_refr.h"
#include "../misc/lv_bidi.h"
#include "../misc/lv_assert.h"

/*********************
 *      DEFINES
 *********************/
#define LABEL_RECOLOR_PAR_LENGTH 6
#define LV_LABEL_HINT_UPDATE_TH 1024 /*Update the "hint" if the label's y coordinates have changed more then this*/

/**********************
 *      TYPEDEFS
 **********************/
enum {
    CMD_STATE_WAIT,
    CMD_STATE_PAR,
    CMD_STATE_IN,
};
typedef uint8_t cmd_state_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static uint8_t hex_char_to_num(char hex);

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






void LV_ATTRIBUTE_FAST_MEM lv_draw_text_dsc_init(lv_draw_text_dsc_t * dsc)
{
    lv_memset_00(dsc, sizeof(lv_draw_text_dsc_t));
    dsc->opa = LV_OPA_COVER;
    dsc->color = lv_color_black();
    dsc->font = LV_FONT_DEFAULT;
    dsc->sel_start = LV_DRAW_TEXT_NO_TXT_SEL;
    dsc->sel_end = LV_DRAW_TEXT_NO_TXT_SEL;
    dsc->sel_color = lv_color_black();
    dsc->sel_bg_color = lv_palette_main(LV_PALETTE_BLUE);
    dsc->bidi_dir = LV_BASE_DIR_LTR;
}

/**
 * Write a text
 * @param coords coordinates of the label
 * @param mask the label will be drawn only in this area
 * @param dsc pointer to draw descriptor
 * @param txt `\0` terminated text to write
 * @param hint pointer to a `lv_draw_label_hint_t` variable.
 * It is managed by the draw to speed up the drawing of very long texts (thousands of lines).
 */
void LV_ATTRIBUTE_FAST_MEM lv_draw_text(lv_draw_ctx_t * draw_ctx, const lv_draw_text_dsc_t * dsc,
                                         const lv_area_t * coords, const char * txt, lv_draw_text_hint_t * hint)
{
    if(dsc->opa <= LV_OPA_MIN) return;//检查不透明属性
    if(dsc->font == NULL) {
        LV_LOG_WARN("dsc->font == NULL");//检查字体
        return;
    }

    if(draw_ctx->draw_letter == NULL) {
        LV_LOG_WARN("draw->draw_letter == NULL (there is no function to draw letters)");//检查是否有提供绘制字母的方法
        return;
    }

    lv_draw_text_dsc_t dsc_mod = *dsc;

    const lv_font_t * font = dsc->font;
    int32_t w;

    /*No need to waste processor time if string is empty*/
    if(txt == NULL || txt[0] == '\0')
        return;

    lv_area_t clipped_area;
    bool clip_ok = _lv_area_intersect(&clipped_area, coords, draw_ctx->clip_area);//计算是否有交集，没有直接返回，直接挡住没显示
    if(!clip_ok) return;

    lv_text_align_t align = dsc->align;
    lv_base_dir_t base_dir = dsc->bidi_dir;//文字基线方向

    lv_bidi_calculate_align(&align, &base_dir, txt);//重新计算文字对齐方式和基线方向
    //处理文本布局的宽度计算问题
    if((dsc->flag & LV_TEXT_FLAG_EXPAND) == 0) {
        /*Normally use the label's width as width*/
        w = lv_area_get_width(coords);
    }
    else {
        /*If EXPAND is enabled then not limit the text's width to the object's width*/
        lv_point_t p;
        lv_txt_get_size(&p, txt, dsc->font, dsc->letter_space, dsc->line_space, LV_COORD_MAX,
                        dsc->flag);
        w = p.x;
    }
    //获取字体的行高+额外的高度=最后的高度
    int32_t line_height_font = lv_font_get_line_height(font);
    int32_t line_height = line_height_font + dsc->line_space;

    /*Init variables for the first line*/
    int32_t line_width = 0;
    lv_point_t pos;
    pos.x = coords->x1;
    pos.y = coords->y1;
    
    //水平和垂直偏移量
    int32_t x_ofs = 0;
    int32_t y_ofs = 0;
    x_ofs = dsc->ofs_x;
    y_ofs = dsc->ofs_y;
    pos.y += y_ofs;
    
    uint32_t line_start     = 0;
    int32_t last_line_start = -1;
    
    //使用缓存信息（hint）来优化重绘过程的一部分
    /*Check the hint to use the cached info*/
    if(hint && y_ofs == 0 && coords->y1 < 0) {
        /*If the label changed too much recalculate the hint.*/
        if(LV_ABS(hint->coord_y - coords->y1) > LV_LABEL_HINT_UPDATE_TH - 2 * line_height) {
            hint->line_start = -1;
        }
        last_line_start = hint->line_start;
    }
    //如果缓存变化不大，基于上次布局的垂直偏移
    /*Use the hint if it's valid*/
    if(hint && last_line_start >= 0) {
        line_start = last_line_start;
        pos.y += hint->y;
    }
    //处理文本布局和换行逻辑的部分
    uint32_t line_end = line_start + _lv_txt_get_next_line(&txt[line_start], font, dsc->letter_space, w, NULL, dsc->flag);

    /*Go the first visible line*/
    while(pos.y + line_height_font < draw_ctx->clip_area->y1) {
        /*Go to next line*/
        line_start = line_end;
        line_end += _lv_txt_get_next_line(&txt[line_start], font, dsc->letter_space, w, NULL, dsc->flag);
        pos.y += line_height;

        /*Save at the threshold coordinate*/
        if(hint && pos.y >= -LV_LABEL_HINT_UPDATE_TH && hint->line_start < 0) {
            hint->line_start = line_start;
            hint->y          = pos.y - coords->y1;
            hint->coord_y    = coords->y1;
        }

        if(txt[line_start] == '\0') return;
    }

    /*Align to middle*/
    if(align == LV_TEXT_ALIGN_CENTER) {
        line_width = lv_txt_get_width(&txt[line_start], line_end - line_start, font, dsc->letter_space, dsc->flag);

        pos.x += (lv_area_get_width(coords) - line_width) / 2;

    }
    /*Align to the right*/
    else if(align == LV_TEXT_ALIGN_RIGHT) {
        line_width = lv_txt_get_width(&txt[line_start], line_end - line_start, font, dsc->letter_space, dsc->flag);
        pos.x += lv_area_get_width(coords) - line_width;
    }
    uint32_t sel_start = dsc->sel_start;
    uint32_t sel_end = dsc->sel_end;
    if(sel_start > sel_end) {
        uint32_t tmp = sel_start;
        sel_start = sel_end;
        sel_end = tmp;
    }
    lv_draw_line_dsc_t line_dsc;

    if((dsc->decor & LV_TEXT_DECOR_UNDERLINE) || (dsc->decor & LV_TEXT_DECOR_STRIKETHROUGH)) {
        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.color = dsc->color;
        line_dsc.width = font->underline_thickness ? font->underline_thickness : 1;
        line_dsc.opa = dsc->opa;
        line_dsc.blend_mode = dsc->blend_mode;
    }

    cmd_state_t cmd_state = CMD_STATE_WAIT;
    uint32_t i;
    uint32_t par_start = 0;
    lv_color_t recolor  = lv_color_black();
    lv_color_t color = lv_color_black();
    int32_t letter_w;

    lv_draw_rect_dsc_t draw_dsc_sel;
    lv_draw_rect_dsc_init(&draw_dsc_sel);
    draw_dsc_sel.bg_color = dsc->sel_bg_color;

    int32_t pos_x_start = pos.x;
    /*Write out all lines   逐行*/
    while(txt[line_start] != '\0') {
        pos.x += x_ofs;

        /*Write all letter of a line*/
        cmd_state = CMD_STATE_WAIT;
        i         = 0;
#if LV_USE_BIDI
        char * bidi_txt = lv_mem_buf_get(line_end - line_start + 1);
        _lv_bidi_process_paragraph(txt + line_start, bidi_txt, line_end - line_start, base_dir, NULL, 0);
#else
        const char * bidi_txt = txt + line_start;
        //printf("bidi_txt : %s\n", bidi_txt);
#endif
        /*对bidi_txt(代表一行字符内容)每个字符做符号标记*/
        int chPtr = 0;//当前读取的字符对应的字符串位置
        int stylePtr = 0;  //每次需要的长度
        int setStyling[line_end - line_start + 1];
        int size = line_end - line_start;
        do{
            char ch = *(bidi_txt + chPtr);
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {  //跳过空白
                stylePtr = 0;
                chPtr++;
                stylePtr++;

            } else if (isOperator(ch)) {
                stylePtr = 0;
                stylePtr++;
                chPtr++;
                set_styling(setStyling, chPtr, stylePtr, OPERATOR);

            } else if (isLetter(ch)) {
                stylePtr = 0;
                if (isKeyLetter(ch)) {  //特殊字母
                    chPtr++;
                    stylePtr++;

                    if (isLetter(*(bidi_txt + chPtr))) {
                        char str[100];  // 假设关键字的最大长度
                        str[0] = bidi_txt[chPtr - 1];

                        while (chPtr < size && isLetter(bidi_txt[chPtr])) {
                            str[strlen(str)] = bidi_txt[chPtr];
                            chPtr++;
                            stylePtr++;
                        }
                        str[strlen(str)] = '\0';  // 终止字符串
                        
                        if (contains(&controlKeywordsSet, str)) {
                            set_styling(setStyling, chPtr, stylePtr, CONTROLKEYWORD);  //控制关键字
                        } else if (contains(&operatorKeywordsSet, str)) {
                            set_styling(setStyling, chPtr, stylePtr, OPERATOR);  //运算符关键字
                        } else if (contains(&macroProgramKeywordsSet, str)) {
                            set_styling(setStyling, chPtr, stylePtr, MACROPROGRAM);  //宏程序关键字
                        } else {
                            set_styling(setStyling, chPtr, stylePtr, ERROR);
                        }

                    } else if (isDigit(*(bidi_txt + chPtr))) {
                        char str[100];  // 假设关键字的最大长度
                        str[0] = bidi_txt[chPtr - 1];

                        while (chPtr < size && isDigit(bidi_txt[chPtr])) {
                            str[strlen(str)] = bidi_txt[chPtr];
                            chPtr++;
                            stylePtr++;
                        }
                        str[strlen(str)] = '\0';  // 终止字符串

                        switch (ch) {
                            case 'G':
                                set_styling(setStyling, chPtr, stylePtr, G);
                                break;

                            case 'M':
                                if (str == "M91" || str == "M92") {
                                    set_styling(setStyling, chPtr, stylePtr, CONTROLKEYWORD);  //控制关键字
                                    break;
                                }
                                set_styling(setStyling, chPtr, stylePtr, M);
                                break;

                            case 'S':
                                set_styling(setStyling, chPtr, stylePtr, S);
                                break;

                            case 'T':
                                set_styling(setStyling, chPtr, stylePtr, T);
                                break;

                            case 'F':
                                set_styling(setStyling, chPtr, stylePtr, F);
                                break;

                            case 'N':
                                if ((chPtr - stylePtr - 1) < 0 || bidi_txt[chPtr - stylePtr - 1] == '\n') {
                                    set_styling(setStyling, chPtr, stylePtr, N);  //作为段号
                                    break;
                                }

                                set_styling(setStyling, chPtr, stylePtr, NROMALLETTTER);  // normal letter
                                break;

                            default:
                                set_styling(setStyling, chPtr, stylePtr, ERROR);
                                break;
                        }
                        
                    } else if (*(bidi_txt + chPtr) == '[' || *(bidi_txt + chPtr) == ' ') {
                        switch (ch) {
                            case 'G':
                                set_styling(setStyling, chPtr, stylePtr, G);
                                break;
                            case 'M':
                                set_styling(setStyling, chPtr, stylePtr, M);
                                break;
                            case 'S':
                                set_styling(setStyling, chPtr, stylePtr, S);
                                break;
                            case 'T':
                                set_styling(setStyling, chPtr, stylePtr, T);
                                break;
                            case 'F':
                                set_styling(setStyling, chPtr, stylePtr, F);
                                break;
                            case 'N':
                                set_styling(setStyling, chPtr, stylePtr, N);
                                break;
                            default:
                                set_styling(setStyling, chPtr, stylePtr, ERROR);
                                break;
                        }
                    } else {
                        set_styling(setStyling, chPtr, stylePtr, ERROR);
                    }
                } else {
                    if (ch == 'C') {
                        stylePtr++;
                        chPtr++;
                        if (*(bidi_txt + chPtr) == 'H') {
                            stylePtr++;
                            chPtr++;
                            while ((chPtr < size) && isDigit(*(bidi_txt + chPtr))) {
                                stylePtr++;
                                chPtr++;
                            }

                            set_styling(setStyling, chPtr, stylePtr, CHANNEL);  //通道
                            continue;
                        } else {
                            chPtr--;
                            stylePtr--;
                        }
                    }
                    if (ch == 'U') {
                        stylePtr++;
                        chPtr++;
                        
                        if (strchr("ABCXYZ", *(bidi_txt + chPtr)) != NULL) {
                            set_styling(setStyling, chPtr, stylePtr, NROMALLETTTER);  // normalLetter
                            continue;
                        } else {
                            chPtr--;
                            stylePtr--;
                        }
                    }
                    chPtr++;
                    stylePtr++;
                    if (isLetter(*(bidi_txt + chPtr))) {
                        char str[100] = {0};  // 假设关键字的最大长度
                        str[0] = bidi_txt[chPtr - 1];
                        printf("Keyword : %s\n", str);
                        while (chPtr < size && (isLetter(bidi_txt[chPtr]) || isDigit(bidi_txt[chPtr]))){
                            str[strlen(str)] = bidi_txt[chPtr];
                            printf("Keyword : %s\n", str);
                            chPtr++;
                            stylePtr++;
                        }
                        str[strlen(str)] = '\0';  // 终止字符串
                        printf("Keyword : %s\n", str);
                        // 正则表达式
                        const char *pattern = "(DO\\d{1,2}\\b|END\\d{1,2}\\b)";

                        if (contains(&controlKeywordsSet, str) || regex_match(pattern, str)) {
                            set_styling(setStyling, chPtr, stylePtr, CONTROLKEYWORD);  //控制关键字
                        } else if (contains(&operatorKeywordsSet, str)) {
                            set_styling(setStyling, chPtr, stylePtr, OPERATOR);  
                        } else if (contains(&macroProgramKeywordsSet, str)) {
                            set_styling(setStyling, chPtr, stylePtr, MACROPROGRAM); 
                        } else {
                            set_styling(setStyling, chPtr, stylePtr, ERROR);
                        }
                    } else {
                        set_styling(setStyling, chPtr, stylePtr, NROMALLETTTER);
                    }
                }
            }else if (ch == '#' || ch == '@') {
                stylePtr = 0;
                chPtr++;
                stylePtr++;
                set_styling(setStyling, chPtr, stylePtr, NORMAL);  // macro
                while ((chPtr < size) && (isDigit(*(bidi_txt + chPtr)) || (isLetter(*(bidi_txt + chPtr))))) {
                    chPtr++;
                    stylePtr++;
                }
                if (stylePtr == 2 && *(bidi_txt + chPtr - 1) == '0') {
                    set_styling(setStyling, chPtr, stylePtr, MACROVAREX);  // #0
                } else if (stylePtr == 5) {
                    //#3000 #3003 #3004
                    char str[100];  // 假设关键字的最大长度
                    str[strlen(str)] = *(bidi_txt + chPtr - 4);
                    str[strlen(str)] = *(bidi_txt + chPtr - 3);
                    str[strlen(str)] = *(bidi_txt + chPtr - 2);
                    str[strlen(str)] = *(bidi_txt + chPtr - 1);
                    str[strlen(str)] = '\0';  // 终止字符串
                    if (strcmp(str, "3000") == 0 || strcmp(str, "3003") == 0 || strcmp(str, "3004") == 0) 
                        set_styling(setStyling, chPtr, stylePtr, MACROVAREX);
                    else
                        set_styling(setStyling, chPtr, stylePtr, MACROVAR);
                } else {
                    set_styling(setStyling, chPtr, stylePtr, MACROVAR);
                }
            } else if (ch == ';') {
                stylePtr = 0;
                stylePtr++;  //开始记录注释长度
                chPtr++;

                while (chPtr < size && *(bidi_txt + chPtr) != '\r') {
                    chPtr++;
                    stylePtr++;
                }
                set_styling(setStyling, chPtr, stylePtr, COMMENT);  // comment
            } else if (ch == '(') {
                stylePtr = 0;
                stylePtr++;  //开始记录注释长度
                chPtr++;

                while ((chPtr < size) && *(bidi_txt + chPtr) != '\r' && *(bidi_txt + chPtr) != ')') {
                    chPtr++;
                    stylePtr++;
                }
                if ((chPtr < size) && (*(bidi_txt + chPtr) == ')')) {
                    chPtr++;
                    stylePtr++;
                }
                set_styling(setStyling, chPtr, stylePtr, COMMENT);  // comment
                continue;
            } else if (ch == '/') {
                stylePtr = 0;
                chPtr++;
                stylePtr++;
                if (*(bidi_txt + chPtr) == '/') {
                    while (chPtr < size && *(bidi_txt + chPtr) != '\r') {
                        chPtr++;
                        stylePtr++;
                    }
                    set_styling(setStyling, chPtr, stylePtr, COMMENT);  // comment
                } else {
                    set_styling(setStyling, chPtr, stylePtr, OPERATOR);  // 运算符
                }
            } else if (ch > 0) {
                stylePtr = 0;
                chPtr++;
                stylePtr++;
                set_styling(setStyling, chPtr, stylePtr, NORMAL);
            } else {
                stylePtr = 0;
                chPtr++;
                stylePtr++;
                set_styling(setStyling, chPtr, stylePtr, ERROR);  // error
            }
        }while (chPtr < size);

        /* 逐字符draw*/
        while(i < line_end - line_start) {
            uint32_t logical_char_pos = 0;
            if(sel_start != 0xFFFF && sel_end != 0xFFFF) {
#if LV_USE_BIDI
                logical_char_pos = _lv_txt_encoded_get_char_id(txt, line_start);
                uint32_t t = _lv_txt_encoded_get_char_id(bidi_txt, i);
                logical_char_pos += _lv_bidi_get_logical_pos(bidi_txt, NULL, line_end - line_start, base_dir, t, NULL);
#else
                logical_char_pos = _lv_txt_encoded_get_char_id(txt, line_start + i);
#endif
            }

            uint32_t letter;
            uint32_t letter_next;
           
           // printf("The value of the letter is: %u\n",i);

            _lv_txt_encoded_letter_next_2(bidi_txt, &letter, &letter_next, &i);//实现i+1
           
           //printf("The value of the letter is: %u, %u\n", letter,letter_next);
            /*Handle the re-color command*/
            if((dsc->flag & LV_TEXT_FLAG_RECOLOR) != 0) {
                if(letter == (uint32_t)LV_TXT_COLOR_CMD[0]) {
                    if(cmd_state == CMD_STATE_WAIT) { /*Start char*/
                        par_start = i;
                        cmd_state = CMD_STATE_PAR;
                        continue;
                    }
                    else if(cmd_state == CMD_STATE_PAR) {   /*Other start char in parameter escaped cmd. char*/
                        cmd_state = CMD_STATE_WAIT;
                    }
                    else if(cmd_state == CMD_STATE_IN) {   /*Command end*/
                        cmd_state = CMD_STATE_WAIT;
                        continue;
                    }
                }

                /*Skip the color parameter and wait the space after it*/
                if(cmd_state == CMD_STATE_PAR) {
                    if(letter == ' ') {
                        /*Get the parameter*/
                        if(i - par_start == LABEL_RECOLOR_PAR_LENGTH + 1) {
                            char buf[LABEL_RECOLOR_PAR_LENGTH + 1];
                            lv_memcpy_small(buf, &bidi_txt[par_start], LABEL_RECOLOR_PAR_LENGTH);
                            buf[LABEL_RECOLOR_PAR_LENGTH] = '\0';
                            int r, g, b;
                            r       = (hex_char_to_num(buf[0]) << 4) + hex_char_to_num(buf[1]);
                            g       = (hex_char_to_num(buf[2]) << 4) + hex_char_to_num(buf[3]);
                            b       = (hex_char_to_num(buf[4]) << 4) + hex_char_to_num(buf[5]);
                            recolor = lv_color_make(r, g, b);
                        }
                        else {
                            recolor.full = dsc->color.full;
                        }
                        cmd_state = CMD_STATE_IN; /*After the parameter the text is in the command*/
                    }
                    continue;
                }
            }

            color = dsc->color;

            if(cmd_state == CMD_STATE_IN) color = recolor;

            letter_w = lv_font_get_glyph_width(font, letter, letter_next);

            if(sel_start != 0xFFFF && sel_end != 0xFFFF) {
                if(logical_char_pos >= sel_start && logical_char_pos < sel_end) {
                    lv_area_t sel_coords;
                    sel_coords.x1 = pos.x;
                    sel_coords.y1 = pos.y;
                    sel_coords.x2 = pos.x + letter_w + dsc->letter_space - 1;
                    sel_coords.y2 = pos.y + line_height - 1;
                    lv_draw_rect(draw_ctx, &draw_dsc_sel, &sel_coords);
                    color = dsc->sel_color;
                }
            }

            dsc_mod.color = color;
            
            switch (setStyling[i]) {
                case NORMAL:
                    dsc_mod.color = lv_color_hex(0x000000); // 黑色
                    break;
                case CONTROLKEYWORD:
                    dsc_mod.color = lv_color_hex(0xFF5733); // 橙色
                    break;
                case COMMENT:
                    dsc_mod.color = lv_color_hex(0x00FF00); // 绿色
                    break;
                case MACROVAR:
                    dsc_mod.color = lv_color_hex(0x0000FF); // 蓝色
                    break;
                case OPERATOR:
                    dsc_mod.color = lv_color_hex(0xFF0000); // 红色
                    break;
                case CHANNEL:
                    dsc_mod.color = lv_color_hex(0xFFFF00); // 黄色
                    break;
                case NROMALLETTTER:
                    dsc_mod.color = lv_color_hex(0x808080); // 灰色
                    break;
                case MACROPROGRAM:
                    dsc_mod.color = lv_color_hex(0x800080); // 紫色
                    break;
                case G:
                    dsc_mod.color = lv_color_hex(0x00FFFF); // 青色
                    break;
                case M:
                    dsc_mod.color = lv_color_hex(0x008000); // 深绿色
                    break;
                case S:
                    dsc_mod.color = lv_color_hex(0x800000); // 深红色
                    break;
                case T:
                    dsc_mod.color = lv_color_hex(0x000080); // 深蓝色
                    break;
                case N:
                    dsc_mod.color = lv_color_hex(0x808000); // 橄榄绿
                    break;
                case F:
                    dsc_mod.color = lv_color_hex(0xFFA500); // 橙黄色
                    break;
                case ERROR:
                    dsc_mod.color = lv_color_hex(0x8B0000); // 深红色
                    break;
                case MACROVAREX:
                    dsc_mod.color = lv_color_hex(0x000080); // 深蓝色
                    break;
                default:
                    dsc_mod.color = lv_color_hex(0x000000); // 黑色
                    break;
            }




            //if(letter == (uint32_t)72) dsc_mod.color = lv_color_hex(0xFF5733);
            lv_draw_letter(draw_ctx, &dsc_mod, &pos, letter);

            if(letter_w > 0) {
                pos.x += letter_w + dsc->letter_space;
            }
        }

        if(dsc->decor & LV_TEXT_DECOR_STRIKETHROUGH) {
            lv_point_t p1;
            lv_point_t p2;
            p1.x = pos_x_start;
            p1.y = pos.y + (dsc->font->line_height / 2)  + line_dsc.width / 2;
            p2.x = pos.x;
            p2.y = p1.y;
            line_dsc.color = color;
            lv_draw_line(draw_ctx, &line_dsc, &p1, &p2);
        }

        if(dsc->decor  & LV_TEXT_DECOR_UNDERLINE) {
            lv_point_t p1;
            lv_point_t p2;
            p1.x = pos_x_start;
            p1.y = pos.y + dsc->font->line_height - dsc->font->base_line - font->underline_position;
            p2.x = pos.x;
            p2.y = p1.y;
            line_dsc.color = color;
            lv_draw_line(draw_ctx, &line_dsc, &p1, &p2);
        }

#if LV_USE_BIDI
        lv_mem_buf_release(bidi_txt);
        bidi_txt = NULL;
#endif
        /*Go to next line*/
        line_start = line_end;
        line_end += _lv_txt_get_next_line(&txt[line_start], font, dsc->letter_space, w, NULL, dsc->flag);

        pos.x = coords->x1;
        /*Align to middle*/
        if(align == LV_TEXT_ALIGN_CENTER) {
            line_width =
                lv_txt_get_width(&txt[line_start], line_end - line_start, font, dsc->letter_space, dsc->flag);

            pos.x += (lv_area_get_width(coords) - line_width) / 2;

        }
        /*Align to the right*/
        else if(align == LV_TEXT_ALIGN_RIGHT) {
            line_width =
                lv_txt_get_width(&txt[line_start], line_end - line_start, font, dsc->letter_space, dsc->flag);
            pos.x += lv_area_get_width(coords) - line_width;
        }

        /*Go the next line position*/
        pos.y += line_height;

        if(pos.y > draw_ctx->clip_area->y2) return;
    }

    LV_ASSERT_MEM_INTEGRITY();
}

/*
void lv_draw_letter(lv_draw_ctx_t * draw_ctx, const lv_draw_label_dsc_t * dsc,  const lv_point_t * pos_p,
                    uint32_t letter)
{
    draw_ctx->draw_letter(draw_ctx, dsc, pos_p, letter);
}
*/


/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Convert a hexadecimal characters to a number (0..15)
 * @param hex Pointer to a hexadecimal character (0..9, A..F)
 * @return the numerical value of `hex` or 0 on error
 */
static uint8_t hex_char_to_num(char hex)
{
    uint8_t result = 0;

    if(hex >= '0' && hex <= '9') {
        result = hex - '0';
    }
    else {
        if(hex >= 'a') hex -= 'a' - 'A'; /*Convert to upper case*/

        switch(hex) {
            case 'A':
                result = 10;
                break;
            case 'B':
                result = 11;
                break;
            case 'C':
                result = 12;
                break;
            case 'D':
                result = 13;
                break;
            case 'E':
                result = 14;
                break;
            case 'F':
                result = 15;
                break;
            default:
                result = 0;
                break;
        }
    }

    return result;
}

