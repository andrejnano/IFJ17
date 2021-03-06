/*
    + ------------- +
    | PROJEKT IFJ17 |
    + ------------- +

    Společný projekt IFJ a IAL 2017
        Implementace prekladace imperativního jazyka IFJ17.

    Varianta zadanie:
        Tým 025, varianta I

    Soubor:
        expression.c

    Autori:
        xmarko15 Peter Marko
        xmechl00 Stanislav Mechl
        xnanoa00 Andrej Nano
        xsvand06 Švanda Jan
*/

#include "expression.h"
#include "symtable.h"
#include "token.h"
#include "parser.h"
#include "instructions.h"

/*
 * \brief Function for converting token literal value to token
 * keword value, for example token_val_double to token_double;
 */
int TvalToKeyword(int val)
{
	switch (val)
	{
		case token_val_double:
			return token_double;
		case token_val_integer:
			return token_integer;
		case token_val_string:
			return token_string;
		default:
			return -1;
	}
	return 0;
}

/*
* \brief Generates new unused name for variable or label
*/
void generateName(char **var)
{
	if (!output_code)
	{
		raise_error(1, "Error : not valid ouput file generateVariable\n");
		return;
	}
	// counter for generating special name
	static int count = 0;
	char *before = *var;
	*var = (char *)malloc(sizeof(*var) + 10);
	// special count number appended to name
	sprintf(*var,"$%s%u", before, count);
   add_inst("DEFVAR", i_tf, *var ,i_null,NULL,i_null,NULL);
	count++;
}

/*
 * \brief Initialisation of operand stack, used in expression evaluation
 */
void sInit(tStack **s)
{
	*s = NULL;
}

 /*
  * \brief Push new operand to top of the stack
  */
void sPush(tStack **s, tStack *val)
{
	tStack *top = (tStack *)malloc(sizeof(tStack));
	memcpy(top, val, sizeof(tStack));
	top->next = *s;
	*s = top;
}

 /*
  * \brief Pop operand from top of stack
  */
tStack *sPop(tStack **s)
{
	tStack *result = *s;
	*s = (*s)->next;
	return result;
}

 /*
  * \brief Generate code fore converting value
  * \param inType Type of source value
  * \param outType Type of destination value
  * \param val Source value to be converted
  * \param frame Mempry frame of source value, NULL if literal
  * \return name of new generated variable, in which is stored
  * converted value, NULL in case of error
  */
char *convert(int inType, int outType, token_value val, char *frame)
{
	char *tmpVar = "convert";
	switch (outType)
	{
	case  token_integer:
		if (inType != token_double)
		{
			raise_error(E_SEM_TYPE, "Wrong type for conversion\n");
			return NULL;
		}
		generateName(&tmpVar);
		if (!frame)
			add_inst("FLOAT2R2EINT", i_tf, tmpVar, i_fl , d2s(val.d), i_null,NULL);
		else
			add_inst("FLOAT2R2EINT", i_tf, tmpVar, fr2in(frame), val.c, i_null,NULL);
	break;
	case  token_double:
		if (inType != token_integer)
		{
			raise_error(E_SEM_TYPE, "Wrong type for conversion\n");
			return NULL;
		}
		generateName(&tmpVar);
		if (!frame)
			add_inst("INT2FLOAT", i_tf, tmpVar, i_int , i2s(val.i), i_null,NULL);
		else
			add_inst("FLOAT2R2EINT", i_tf, tmpVar, fr2in(frame) , val.c, i_null,NULL);
	break;
	default:
		if (istype(active_token->type))
			raise_error(E_SEM_TYPE, "Wrong type for conversion\n");
		return NULL;
	}
	return tmpVar;
}

/*
 * \brief Generating ifj17code for initialization of new var
 * \param varName Name of variable to be initialized
 */
void zeroVarInit(char *varName) {
	switch (active_token->type)
	{
		case token_integer:
			add_inst("MOVE", i_lf, varName, i_int , "0", i_null,NULL);
			break;
		case token_double:
			add_inst("MOVE", i_lf, varName, i_fl , "0.0", i_null,NULL);
			break;
		case token_string:
			add_inst("MOVE", i_lf, varName, i_str , "", i_null,NULL);
			break;
		case token_boolean:
			add_inst("MOVE", i_lf, varName, i_bool , "false", i_null,NULL);
			break;
		default :
			raise_error(E_SYNTAX, "Expecet type int, double ...\n");
	}
}

/*
 * \brief Converting values stored at stack from type T1 to new_type
 * \param new_type Type of second value in stack
 * \param old_type type of older inserted value in stack
 * \param by_priority convert to type with more priority = float
 *  if 0 convert from new_type to old_type
 */
void converts(int new_type, int *old_type, bool byPriority)
{
	if (istype(new_type) && istype(new_type))
	{
		if (new_type == *old_type)	// no need for conversion
			return;
		if (new_type == token_double && *old_type == token_integer)
		{
			if (byPriority)
			{
				// older type at the stack is int so need to pop and then convert
				char *temp = "tmp";
				generateName(&temp);
				add_inst("POPS", i_tf, temp, i_null,NULL,i_null,NULL);
				add_inst("INT2FLOATS", i_null,NULL,i_null,NULL,i_null,NULL);
				add_inst("PUSHS", i_tf, temp, i_null,NULL,i_null,NULL);
				*old_type = token_double;
			}
			else
			{
				add_inst("FLOAT2R2EINTS", i_null,NULL,i_null,NULL,i_null,NULL);
				*old_type = token_integer;
			}
		}
		else if (*old_type == token_double && new_type == token_integer)
		{
			// same for byPriority or !byPriority
			add_inst("INT2FLOATS", i_null,NULL,i_null,NULL,i_null,NULL);
			*old_type = token_double;
		}
		else
		{
			// cannot convert other types then float -> int or int -> float
			raise_error(E_SEM_TYPE, "Wrong type for conversion\n");
			return;
		}
	}
	else	// trying to convert something which is not data type
	{
		raise_error(E_SYNTAX, "Wrong expression syntax\n");
		return;
	}
}

/*
 * \brief Test if types are valid for comparision
 * \param OP1 type of first compared operand
 * \param OP2 type of second compared operand
 */
void testCmpOps(int OP1, int *OP2)
{
	if (!istype(OP1) || !istype(*OP2))
	{
		raise_error(E_SYNTAX, "Unexpected token\n");
		return;
	}
	if (OP1 == *OP2)
	{
		return;
	}
	else if (*OP2 == token_integer && OP1 == token_double)
	{
		add_inst("INT2FLOATS", i_null,NULL,i_null,NULL,i_null,NULL);
		*OP2 = token_double;
	}
	else if (OP1 == token_integer && *OP2 == token_double)
	{
		char *temp1 = "tmp";
		generateName(&temp1);
		add_inst("POPS", i_tf, temp1, i_null,NULL,i_null,NULL);
		add_inst("INT2FLOATS", i_null,NULL,i_null,NULL,i_null,NULL);
		add_inst("PUSHS", i_tf, temp1, i_null,NULL,i_null,NULL);
		*OP2 = token_double;
	}
	else
	{
		// comparing uncompatible types
		raise_error(E_SEM_TYPE, "Wrong type for comparison\n");
	}
}
/*
 * \brief Function for executing operation from top of stack
 * \param s Stack of operations
 * \param numOp Number of operators in stack
 * \param old_type type of prev operator
 * \return Executed operation, in case of error operation
 * with priority STACK_STOPPER
 */
tStack execOp (tStack **s, int *numOp, int *old_type)
{
	bool int_div; // for conversion after integer division
	tStack tmp; // stack of operators
	tmp.priority = STACK_STOPPER; //set unused value to determine end of stack is reached
	
	tStack *operation = sPop(s); // current operation to be executed
	if (*numOp < 2 && operation->type != token_lbrace)
	{
		// not enaugh operands
		tmp.priority = STACK_STOPPER;
		tmp.type = STACK_STOPPER;
		(*numOp)--;
		return tmp;
	}
	// temporal vars for partial calculations
	char *tmpName1 = "tmpName";
	char *tmpName2 = "tmpName";
	// generate specific code for each operation
	switch (operation->type)
	{
		case token_op_add:
			if (*old_type == token_string)
			{
				// concatenation of strings
				generateName(&tmpName1);
				generateName(&tmpName2);
				add_inst("POPS", i_tf, tmpName1, i_null,NULL,i_null,NULL);
				add_inst("POPS", i_tf, tmpName2, i_null,NULL,i_null,NULL);
				add_inst("CONCAT", i_tf, tmpName1, i_tf, tmpName2, i_tf, tmpName1);
				add_inst("PUSHS", i_tf, tmpName1, i_null,NULL,i_null,NULL);
				free(tmpName1);
				free(tmpName2);
				break;
			}
			converts(operation->lOperandType, old_type, 1);
			add_inst("ADDS", i_null,NULL,i_null,NULL,i_null,NULL);
		break;
		case token_op_sub:
			converts(operation->lOperandType, old_type, 1);
			add_inst("SUBS", i_null,NULL,i_null,NULL,i_null,NULL);
		break;  
		case token_op_mul:
			converts(operation->lOperandType, old_type, 1);
			add_inst("MULS", i_null,NULL,i_null,NULL,i_null,NULL);
		break;  
		case token_op_div:
			int_div = false;
			// in case both are integers we have to convert first manually
			if (operation->lOperandType == token_integer)
			{
				add_inst("INT2FLOATS", i_null, NULL, i_null,NULL,i_null,NULL);
				if (*old_type == token_integer)
					int_div = true;
			}
			converts(token_double, old_type, 1);
			if (*old_type == token_string)
				raise_error(E_SEM_TYPE, "Invalid operation for strings\n");
			add_inst("DIVS", i_null,NULL,i_null,NULL,i_null,NULL);
			if (int_div)
				add_inst("FLOAT2INTS", i_null,NULL,i_null,NULL,i_null,NULL);
		break;  
		case token_op_mod:
			converts(operation->lOperandType, old_type, 1);
			add_inst("FLOAT2R2EINTS", i_null,NULL,i_null,NULL,i_null,NULL);
			add_inst("INT2FLOATS", i_null,NULL,i_null,NULL,i_null,NULL);
			add_inst("DIVS", i_null,NULL,i_null,NULL,i_null,NULL);
			add_inst("FLOAT2INTS", i_null,NULL,i_null,NULL,i_null,NULL);
			add_inst("INT2FLOATS", i_null,NULL,i_null,NULL,i_null,NULL);
		break;
		case token_op_eq:
			testCmpOps(operation->lOperandType, old_type);
			add_inst("EQS", i_null,NULL,i_null,NULL,i_null,NULL);
			*old_type = token_boolean;
		break;
		case token_op_lt:
			testCmpOps(operation->lOperandType, old_type);
			add_inst("LTS", i_null,NULL,i_null,NULL,i_null,NULL);
			*old_type = token_boolean;
		break;
		case token_op_le:
			testCmpOps(operation->lOperandType, old_type);
			generateName(&tmpName1);
			generateName(&tmpName2);
			add_inst("POPS", i_tf, tmpName1, i_null,NULL,i_null,NULL);
			add_inst("POPS", i_tf, tmpName2, i_null,NULL,i_null,NULL);
			add_inst("PUSHS", i_tf, tmpName2, i_null,NULL,i_null,NULL);
			add_inst("PUSHS", i_tf, tmpName1, i_null,NULL,i_null,NULL);
			add_inst("LTS", i_null,NULL,i_null,NULL,i_null,NULL);
			add_inst("PUSHS", i_tf, tmpName2, i_null,NULL,i_null,NULL);
			add_inst("PUSHS", i_tf, tmpName1, i_null,NULL,i_null,NULL);
			add_inst("EQS", i_null,NULL,i_null,NULL,i_null,NULL);
			add_inst("ORS", i_null,NULL,i_null,NULL,i_null,NULL);
			*old_type = token_boolean;
		break;
		case token_op_ge:
			testCmpOps(operation->lOperandType, old_type);
			generateName(&tmpName1);
			generateName(&tmpName2);
			add_inst("POPS", i_tf, tmpName1, i_null,NULL,i_null,NULL);
			add_inst("POPS", i_tf, tmpName2, i_null,NULL,i_null,NULL);
			add_inst("PUSHS", i_tf, tmpName2, i_null,NULL,i_null,NULL);
			add_inst("PUSHS", i_tf, tmpName1, i_null,NULL,i_null,NULL);
			add_inst("GTS", i_null,NULL,i_null,NULL,i_null,NULL);
			add_inst("PUSHS", i_tf, tmpName2, i_null,NULL,i_null,NULL);
			add_inst("PUSHS", i_tf, tmpName1, i_null,NULL,i_null,NULL);
			add_inst("EQS", i_null,NULL,i_null,NULL,i_null,NULL);
			add_inst("ORS", i_null,NULL,i_null,NULL,i_null,NULL);
			*old_type = token_boolean;
		break;
		case token_op_ne:
			testCmpOps(operation->lOperandType, old_type);
			generateName(&tmpName1);
			generateName(&tmpName2);
			add_inst("POPS", i_tf, tmpName1, i_null,NULL,i_null,NULL);
			add_inst("POPS", i_tf, tmpName2, i_null,NULL,i_null,NULL);
			add_inst("PUSHS", i_tf, tmpName2, i_null,NULL,i_null,NULL);
			add_inst("PUSHS", i_tf, tmpName1, i_null,NULL,i_null,NULL);
			add_inst("LTS", i_null,NULL,i_null,NULL,i_null,NULL);
			add_inst("PUSHS", i_tf, tmpName2, i_null,NULL,i_null,NULL);
			add_inst("PUSHS", i_tf, tmpName1, i_null,NULL,i_null,NULL);
			add_inst("GTS", i_null,NULL,i_null,NULL,i_null,NULL);
			add_inst("ORS", i_null,NULL,i_null,NULL,i_null,NULL);
			*old_type = token_boolean;
		break;
		case token_op_gt:
			testCmpOps(operation->lOperandType, old_type);
			add_inst("GTS", i_null,NULL,i_null,NULL,i_null,NULL);
			*old_type = token_boolean;	
		break;
		case token_lbrace:
			free(sPop(s));
		break;
	}
	tmp = *operation;
	free(operation);
	(*numOp) -= 1;
	return tmp;
}

/*
 * \brief Function for generating code for function call
 * \param funcMeta Metadata about function declaration like type of function
 * and types of arguments
 * \param funcname Name of function to be called
 */
void NT_CallExpr(Metadata_t *funcMeta, char *funcName)
{
	get_next_token(source_code, active_token);
	if (!match(token_lbrace))
	{
		raise_error(E_SEM_DEF, "Left bracket expected for function call");
		return;
	}
	add_inst("CREATEFRAME", i_null,NULL,i_null,NULL,i_null,NULL);
	Parameter_t *arg = funcMeta->parameters;
	// read check types and convert arguments for calling function
	while (active_token->type != token_rbrace)
	{
		if (!arg)	// no right bracket but no more arguments in function declaration
		{
			raise_error(E_SEM_TYPE, "Too many arguments for function\n");
			return;
		}
		// evaluate argument
		NT_Expr(arg->type);
		add_inst("DEFVAR", i_tf, arg->name, i_null,NULL,i_null,NULL);
		add_inst("POPS", i_tf, arg->name, i_null, NULL, i_null,NULL);
		arg = arg->next;
		if (active_token->type != token_comma)
			break;
		get_next_token(source_code, active_token);
	}
	if (active_token->type == token_rbrace)
	{
		get_next_token(source_code, active_token);
	}

	if (arg)
	{
		raise_error(E_SEM_TYPE, "Too few arguments for function\n");
		return;
	}
	add_inst("PUSHFRAME", i_null,NULL,i_null,NULL,i_null,NULL);
	add_inst("CALL", i_null, funcName, i_null,NULL,i_null,NULL);
	add_inst("POPFRAME", i_null,NULL,i_null,NULL,i_null,NULL);
	add_inst("PUSHS", i_tf, "%retval", i_null,NULL,i_null,NULL);
	add_inst("CREATEFRAME", i_null,NULL,i_null,NULL,i_null,NULL);
}

/*
 * \brief Function for generating code for expression evaluation
 * \param type Expected expression type, 0 if not expected type for print
 */
void NT_Expr(int type)
{
	tStack *s;
	sInit(&s);
	tStack insert;
	insert.priority = STACK_STOPPER;
	insert.type = STACK_STOPPER;
	sPush(&s, &insert); // insert stack stoper for indication of last operator
	int numOp = 0, // number of operands
	new_type = 0; // variable for string actual type of result of executed operation
	Metadata_t *tmp_meta; // temporal variable for storing function
				// and variable metadata from symtable
	bool not_int = false; // true if final result will not be of type int
	while (active_token->type != token_eol)
	{
		// generating code of expression case for eche token which can occure in expresion
		switch (active_token->type)
		{
		// operators
		case token_op_add:
		case token_op_sub:
			insert.type = active_token->type;
         insert.priority = 3;
         insert.lOperandType = new_type;
			while (s && s->priority && s->priority <= insert.priority)
				if (execOp(&s, &numOp, &new_type).priority == STACK_STOPPER)
					return;
			sPush(&s, &insert);
			break;
		case token_op_div:
			not_int = true;
		case token_op_mul:
			insert.type = active_token->type;
			insert.priority = 1;
         insert.lOperandType = new_type;
			while (s && s->priority && s->priority <= insert.priority)
				if (execOp(&s, &numOp, &new_type).priority == STACK_STOPPER)
					return;
			sPush(&s, &insert);

			break;
		case token_op_mod:
			insert.type = active_token->type;
			insert.priority = 2;
         insert.lOperandType = new_type;
			while (s && s->priority && s->priority <= insert.priority)
				if (execOp(&s, &numOp, &new_type).priority == STACK_STOPPER)
					return;
			sPush(&s, &insert);
			break;
		case token_op_eq:
		case token_op_le:
		case token_op_lt:
		case token_op_ge:
		case token_op_ne:
		case token_op_gt:
			if (type != token_boolean)
			{
				raise_error(E_SEM_TYPE, "Error at line %d : Expected boolean\n");
				return;
			}
			insert.lOperandType = new_type;
			insert.type = active_token->type;
			insert.priority = 4;
			while (s && s->priority && s->priority <= insert.priority)
				if (execOp(&s, &numOp, &new_type).priority == STACK_STOPPER)
					return;
			sPush(&s, &insert);
			break;
		// brackets
		case token_lbrace:
			insert.type = active_token->type;
			insert.priority = 6;
			sPush(&s, &insert);
			break;
		case token_rbrace:
			while (s && s->type != token_lbrace)
			{
				if (execOp(&s, &numOp, &new_type).priority == STACK_STOPPER)
				{
					converts(new_type, &type, 0);
					return;
				}
			}
			free(sPop(&s));
			break;
		// IDs and literals
		case token_identifier:
		 	numOp++;
			if ((tmp_meta = stl_search(variables, active_token->value.c)))
			{
				if (tmp_meta->type != token_integer)
					not_int = true;
            new_type = tmp_meta->type;
				add_inst("PUSHS", i_lf, active_token->value.c, i_null,NULL,i_null,NULL);
				if (new_type == token_integer)
				{
					add_inst("INT2FLOATS", i_null, NULL, i_null,NULL,i_null,NULL);
					new_type = token_double;
				}
			}
			else if ((tmp_meta = stl_search(functions, active_token->value.c)))
			{
				if (tmp_meta->type != token_integer)
					not_int = true;
            new_type = tmp_meta->type;
				NT_CallExpr(tmp_meta, active_token->value.c);
				if (new_type == token_integer)
				{
					add_inst("INT2FLOATS", i_null, NULL, i_null,NULL,i_null,NULL);
					new_type = token_double;
				}
				continue;
			}
			else
			{
				raise_error(E_SEM_DEF, "Unknown identifier\n");
			}

			break;
		case token_val_integer:
			numOp++;
			add_inst("PUSHS", i_null, NULL, i_null,NULL,i_null,NULL);
			printTokenVal();
			add_inst("INT2FLOATS", i_null, NULL, i_null,NULL,i_null,NULL);
			new_type = token_double;
			break;
		case token_val_double:
			numOp++;
			not_int = true;
			add_inst("PUSHS", i_null, NULL, i_null,NULL,i_null,NULL);
			printTokenVal();
			new_type = token_double;
			break;
		case token_val_string:
         	numOp++;
			not_int = true;
			new_type = token_string;
            add_inst("PUSHS", i_null,NULL,i_null,NULL,i_null,NULL); 
			printTokenVal();
			break;
		// built in functions
    	case token_length:
    	    builtin_length_used = true;      
			numOp++;        
    	    if (builtin_length_meta.type != token_integer)
				not_int = true;
    	    new_type = builtin_length_meta.type;                
    	    NT_CallExpr(&builtin_length_meta, "Length");        
			if (new_type == token_integer)
			{
				add_inst("INT2FLOATS", i_null, NULL, i_null,NULL,i_null,NULL);
				new_type = token_double;
			}
    	    continue;
    	case token_substr:
    	    builtin_substr_used = true;      
			numOp++;        
    	    if (builtin_substr_meta.type != token_integer)
				not_int = true;
    	    new_type = builtin_substr_meta.type;                
    	    NT_CallExpr(&builtin_substr_meta, "SubStr");        
			if (new_type == token_integer)
			{
				add_inst("INT2FLOATS", i_null, NULL, i_null,NULL,i_null,NULL);
				new_type = token_double;
			}
    	    continue;
    	case token_asc:
    	    builtin_asc_used = true;      
		 	numOp++;        
    	    if (builtin_asc_meta.type != token_integer)
				not_int = true;
    	    new_type = builtin_asc_meta.type;                
    	    NT_CallExpr(&builtin_asc_meta, "Asc");        
			if (new_type == token_integer)
			{
				add_inst("INT2FLOATS", i_null, NULL, i_null,NULL,i_null,NULL);
				new_type = token_double;
			}
    	    continue;
    	case token_chr:
    	    builtin_chr_used = true;      
		 	numOp++;        
    	    if (builtin_chr_meta.type != token_integer)
				not_int = true;
    	    new_type = builtin_chr_meta.type;                
    	    NT_CallExpr(&builtin_chr_meta, "Chr");        
			if (new_type == token_integer)
			{
				add_inst("INT2FLOATS", i_null, NULL, i_null,NULL,i_null,NULL);
				new_type = token_double;
			}
			continue;
		// unexpected token for expreession end of expression
		default:
			if (istype(active_token->type))
				raise_error(E_SEM_TYPE, "Wrong type in expression\n");
			// execute operations from stack
			while (s && s->priority < STACK_STOPPER)
			{
				if (execOp(&s, &numOp, &new_type).priority == STACK_STOPPER)
				{
					if (!numOp)
						raise_error(E_SYNTAX, "Wrong expression syntax\n");
					return;
				}
			}
			if (numOp != 1 || s->priority != STACK_STOPPER)
			{
				raise_error(E_SYNTAX, "Wrong expression syntax - A\n");
				return;
			}
			if (!type && !not_int)
				type = token_integer;
			if (type && type != new_type)
				converts(new_type, &type, 0);
			free(sPop(&s));		
			return;
		}

		get_next_token(source_code, active_token);
	}
	// execute operations from stack and end of expression
	while (s && s->priority < STACK_STOPPER)
	{
		if (execOp(&s, &numOp, &new_type).priority == STACK_STOPPER)
		{
			if (!numOp)
				raise_error(E_SYNTAX, "Wrong expression syntax\n");
			return;
		}
	}
	if (numOp != 1 || s->priority != STACK_STOPPER)
	{
		raise_error(E_SYNTAX, "Wrong expression syntax - B\n");
		return;
	}
	if (!type && !not_int)
		type = token_integer;
	if (type && type != new_type)
		converts(new_type, &type, 0);
	free(sPop(&s));
}
