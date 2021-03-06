/*
    + ------------- +
    | PROJEKT IFJ17 |
    + ------------- +

    Společný projekt IFJ a IAL 2017
        Implementace prekladace imperativního jazyka IFJ17.

    Varianta zadanie:
        Tým 025, varianta I

    Soubor:
        token.c

    Autori:
        xmarko15 Peter Marko
        xmechl00 Stanislav Mechl
        xnanoa00 Andrej Nano
        xsvand06 Švanda Jan
*/

#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"
#include "token.h"
#include "errorcodes.h"
#include "instructions.h"

extern FILE* source_code;
extern FILE *output_code;
extern Token_t* active_token;

/*
  * \brief Reads token from input and dispose current token
  */
void get_token_free()
{
    if (active_token)
    {
        free(active_token);
    }
    get_next_token(source_code, active_token);
}

/*
    * \brief Checks if token type is int, double, bool, string
    */
bool is_datatype(int type)
{
    if (type != token_integer && type != token_double &&
        type != token_boolean && type != token_string)
        return false;
    return true;
}

/*
    * \brief Print value of token literal to output file with prefix:
    * float@ int@ ...
    */
void printTokenVal(void)
{
    switch (active_token->type)
    {
    case token_val_double:
	add_op_to_last_inst(i_fl, d2s(active_token->value.d));
        //fprintf(output_code, "float@%lf\n", active_token->value.d);
        break;
    case token_val_integer:
	add_op_to_last_inst(i_int, i2s(active_token->value.i));
        //fprintf(output_code, "int@%d\n", active_token->value.i);
        break;
    case token_val_string:
	add_op_to_last_inst(i_str, active_token->value.c);
        //fprintf(output_code, "string@%s\n", active_token->value.c);
        break;
    default:
        fprintf(stderr, "Error at line %d cannot print this type\n", active_token->line);
        return;
    }
}


/******************************************************************************
    TOKEN MATCH 
 *****************************************************************************/

/*
	 * \brief	Checks if the current token is the expected one.
	 * 			If it is. It either frees the token and its memory
	 * 			or just moves on. In case of EOF, the function gets
	 * 			blocked for futher token getting. Just prints the error message.
	 * \param	expected_token_type The expected token by syntax
	 * \return	bool value, wether the token was found or not
	 * 			
	 * 			@TODO refactor a bit
	 */

bool match(token_type expected_token_type)
{

    // guard flag that, if switched on, forbids further reading from scanner
    // and therefore prevents wrong memory access.
    // switches on when at least one of these events happen:
    //	1) the scanner returns false, signaling erro
    //  2) the token is EOF, which is unexpected until the end of program
    // --------------------------------------------------------------------
    static bool block_scanner = false;
    
    if (block_scanner)
    {
        // not exactly right, @TODO change this
        if (expected_token_type == token_eof)
        {
            #ifdef DEBUG
            printf(ANSI_COLOR_YELLOW "[DEBUG]" ANSI_COLOR_RESET " match %s\n\n\t--- END ---\n\n", debug_token_name(active_token));
            #endif

            return true;
        }
        
        raise_error(E_SYNTAX, "Unexpected EOF.");
        return false;
    }
    else
    {
        // if the token exists and its type is the expected type
        if (active_token && active_token->type == expected_token_type)
        {
            
            // if the token is string type, free the char array
            if (active_token->type == token_val_string || active_token->type == token_identifier)
                free(active_token->value.c);

            // free the memory allocated by token
            //free(active_token);

            #ifdef DEBUG
            printf(ANSI_COLOR_YELLOW "[DEBUG]" ANSI_COLOR_RESET " match %s\n", debug_token_name(active_token));
            #endif

            // if error occurs in scanner,
            if (get_next_token(source_code, active_token) == false || active_token->type == token_eof)
            {
                block_scanner = true;
            }

            return true;
        }
        else
        {

            // if the token is string type, free the char array
            if (active_token->type == token_val_string || active_token->type == token_identifier)
                free(active_token->value.c);

            // free the memory allocated by token
            //free(active_token);

            #ifdef DEBUG
            printf(ANSI_COLOR_YELLOW "[DEBUG]" ANSI_COLOR_RESET " unexpected %s\n", debug_token_name(active_token));
            #endif

            // if error occurs in scanner,
            if (get_next_token(source_code, active_token) == false || active_token->type == token_eof)
            {
                block_scanner = true;
            }

            return false;
        }
    }
}

/**
 * @brief Function for better and more efficient debugging
 *  Outputs the current token_type in formated style
 * @param token the token to be printed out
 */

const char *debug_token_name(Token_t *token)
{
    token_type type = token->type;
    if (!token)
        return NULL;
    switch (type)
    {
    // Data types
    case token_blank:
        return "token_blank";
    case token_identifier:
    {
        char *buffer = (char *)malloc(sizeof(char) * 100);
        sprintf(buffer, "token_identifier >> " ANSI_COLOR_MAGENTA "'%s'" ANSI_COLOR_RESET, token->value.c);
        return buffer;
    }
    case token_val_integer:
    {
        char *buffer = (char *)malloc(sizeof(char) * 100);
        sprintf(buffer, "token_val_integer >> " ANSI_COLOR_BLUE "'%d'" ANSI_COLOR_RESET, token->value.i);
        return buffer;
    }
    case token_val_double:
    {
        char *buffer = (char *)malloc(sizeof(char) * 100);
        sprintf(buffer, "token_val_double >> " ANSI_COLOR_BLUE "'%f'" ANSI_COLOR_RESET, token->value.d);
        return buffer;
    }
    case token_val_string:
    {
        char *buffer = (char *)malloc(sizeof(char) * 100);
        sprintf(buffer, "token_val_string >> " ANSI_COLOR_BLUE "'%s'" ANSI_COLOR_RESET, token->value.c);
        return buffer;
    }

    // Operators
    case token_op_add:
        return "token_op_add";
    case token_op_sub:
        return "token_op_sub";
    case token_op_mul:
        return "token_op_mul";
    case token_op_div:
        return "token_op_div";
    case token_op_mod:
        return "token_op_mod";
    case token_op_lt:
        return "token_op_lt";
    case token_op_gt:
        return "token_op_gt";
    case token_op_le:
        return "token_op_le";
    case token_op_ge:
        return "token_op_ge";
    case token_op_eq:
        return "token_op_eq";
    case token_op_ne:
        return "token_op_ne";

    // Special
    case token_semicolon:
        return "token_semicolon";
    case token_comma:
        return "token_comma";
    case token_lbrace:
        return "token_lbrace";
    case token_rbrace:
        return "token_rbrace";

    // Keywords
    case token_and:
        return "token_and";
    case token_as:
        return "token_as";
    case token_asc:
        return "token_asc";
    case token_boolean:
        return "token_boolean";
    case token_chr:
        return "token_chr";
    case token_continue:
        return "token_continue";
    case token_declare:
        return "token_declare";
    case token_dim:
        return "token_dim";
    case token_do:
        return "token_do";
    case token_double:
        return "token_double";
    case token_else:
        return "token_else";
    case token_elseif:
        return "token_elseif";
    case token_end:
        return "token_end";
    case token_exit:
        return "token_exit";
    case token_false:
        return "token_false";
    case token_for:
        return "token_for";
    case token_function:
        return "token_function";
    case token_if:
        return "token_if";
    case token_input:
        return "token_input";
    case token_integer:
        return "token_integer";
    case token_length:
        return "token_length";
    case token_loop:
        return "token_loop";
    case token_next:
        return "token_next";
    case token_not:
        return "token_not";
    case token_or:
        return "token_or";
    case token_print:
        return "token_print";
    case token_return:
        return "token_return";
    case token_scope:
        return "token_scope";
    case token_shared:
        return "token_shared";
    case token_static:
        return "token_static";
    case token_string:
        return "token_string";
    case token_substr:
        return "token_substr";
    case token_then:
        return "token_then";
    case token_true:
        return "token_true";
    case token_while:
        return "token_while";

    // Read state
    case token_eol:
        return "token_eol";
    case token_eof:
        return "token_eof";
    }

    // Not found
    return "token_not_found";
}

bool istype(int type)
{
	if (type != token_integer && type != token_double &&
		type != token_boolean && type != token_string) 
		return false;
	return true;
}
