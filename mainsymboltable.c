/*
BATCH NO. 27
Mayank Agarwal (2014A7PS111P)
Karan Deep Batra(2014A7PS160P)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "symboltable.h"
#include "hashtable.h"
#include "token.h"
#include "parserDef.h"
#include "mainsymboltable.h"
#include "idsymboltable.h"

idsymboltable* currsymboltable = NULL;
char* func_name;

idsymboltable* checkScope(idsymboltable* currIdst, char* idlex)
{
	if(currIdst == NULL)	
		return NULL;
	if(getidsymboltablenode(idlex, currIdst) != NULL)
		return currIdst;
	return checkScope(currIdst->parent, idlex);
}

int inSameScope(idsymboltable* currIdst, char* idlex)
{
	if(currIdst == NULL)
		return 0;
	if(getidsymboltablenode(idlex, currIdst) != NULL)
		return 1;
	return 0;
}

int getarrayrange(stacknode* type) //type is pointer to ARRAY in AST
{
	int l = atoi(type->child->child->tokinfo->lexeme); //child of ARRAY is <range> and its child is NUM (l)
	int r = atoi(type->child->child->sibling->tokinfo->lexeme); //next sibling is NUM(r)
	return r-l+1;
}

int getlengthofid(stacknode* type)
{
	if(strcmp(type->ntortinfo->str, "INTEGER") == 0)
		return 2;
	if(strcmp(type->ntortinfo->str, "REAL") == 0)
		return 4;
	if(strcmp(type->ntortinfo->str, "BOOLEAN") == 0)
		return 1;
	
	return getarrayrange(type)*getlengthofid(type->child->sibling);
}

void populateExpression(stacknode* curr, idsymboltable* currIdst, int toPrint)
{
	if(curr == NULL)
		return;

	populateExpression(curr->child, currIdst, toPrint);

	// itself
	if(strcmp(curr->ntortinfo->str, "ID") == 0)
	{
		idsymboltable* temp = checkScope(currIdst, curr->tokinfo->lexeme);
		if(temp == NULL)
		{
			semanticCorrect = 1;
			if(toPrint){
				printf("ERROR_V at line %d : %s not declared in this scope.\n", curr->tokinfo->linenumber, curr->tokinfo->lexeme);
			}
		}
		else
			curr->idst = temp;
	}

	if(curr->child == NULL)	
		return;

	stacknode* temp = curr->child->sibling;
	while(temp != NULL)
	{
		populateExpression(temp, currIdst, toPrint);
		temp = temp->sibling;
	}

	return;
}

void populateStatements(stacknode* curr, idsymboltable* currIdst, mainsymboltable* globaltable, int toPrint)
{
	if(curr == NULL)
		return;

	if(strcmp(curr->ntortinfo->str, "<ioStmt>") == 0)
	{
		if(strcmp(curr->child->ntortinfo->str, "GET_VALUE") == 0)
		{
			idsymboltable* temp = checkScope(currIdst, curr->child->sibling->tokinfo->lexeme);
			if(temp == NULL)
			{
				semanticCorrect = 1;
				if(toPrint){
					printf("ERROR_V at line %d : %s not declared in this scope.\n", curr->child->sibling->tokinfo->linenumber, curr->child->sibling->tokinfo->lexeme);
				}
			}
			else
				curr->child->sibling->idst = temp;		// setting symbol table link for this ID
		}
		else
		{
			if(strcmp(curr->child->sibling->child->ntortinfo->str, "ID") == 0)
			{
				idsymboltable* temp = checkScope(currIdst, curr->child->sibling->child->tokinfo->lexeme);
				if(temp == NULL)
				{
					semanticCorrect = 1;
					if(toPrint){
						printf("ERROR_V at line %d : %s not declared in this scope.\n", curr->child->sibling->child->tokinfo->linenumber, curr->child->sibling->child->tokinfo->lexeme);
					}
				}
				else
					curr->child->sibling->child->idst = temp;	// setting symbol table link for this ID
				
				if(curr->child->sibling->child->sibling->child != NULL) // whichId's child ID
				{
					idsymboltable* temp2 = checkScope(currIdst, curr->child->sibling->child->sibling->child->tokinfo->lexeme);
					if(temp2 == NULL)
					{
						semanticCorrect = 1;
						if(toPrint){
							printf("ERROR_V at line %d : %s not declared in scope.\n", curr->child->sibling->child->sibling->child->tokinfo->linenumber, curr->child->sibling->child->sibling->child->tokinfo->lexeme);
						}
					}
					else
						curr->child->sibling->child->sibling->child->idst = temp2;
				}
			}
		}
	}
	else if(strcmp(curr->ntortinfo->str, "<declareStmt>") == 0)
	{
		stacknode* temp = curr->child->child;	// pointing to ID
		while(temp != NULL)
		{
			if(inSameScope(currIdst, temp->tokinfo->lexeme))
			{
				semanticCorrect = 1;
				if(toPrint){
					printf("ERROR_V at line %d : %s already declared in this scope.\n", temp->tokinfo->linenumber, temp->tokinfo->lexeme);
				}
			}
			else
			{
				idsymboltablenode* pt = insertidsymboltablenode(temp->tokinfo->lexeme, curr->child->sibling, currIdst->offset, currIdst);	//curr->child->sibling is type, offset not considered
				temp->idst = currIdst;	// setting symbol table link for this ID
				pt->widthofid = getlengthofid(pt->type);
				currIdst->offset += pt->widthofid;
			}
			temp = temp->sibling;
		}
	}
	else if(strcmp(curr->ntortinfo->str, "<iterativeStmt>") == 0)
	{
		if(strcmp(curr->child->ntortinfo->str, "FOR") == 0)
		{
			idsymboltable* temp = checkScope(currIdst, curr->child->sibling->tokinfo->lexeme);
			if(temp == NULL)
			{
				semanticCorrect = 1;
				if(toPrint){
					printf("ERROR_V at line %d : %s not declared in this scope.\n", curr->child->sibling->tokinfo->linenumber, curr->child->sibling->tokinfo->lexeme);
				}
			}
			else
				curr->child->sibling->idst = temp;	// setting symbol table link for this ID

			idsymboltable* newIdst = makeidsymboltable();
			newIdst->startline = curr->child->sibling->sibling->sibling->tokinfo->linenumber;
			newIdst->endline = curr->child->sibling->sibling->sibling->sibling->sibling->tokinfo->linenumber;
			newIdst->nestinglevel = currIdst->nestinglevel + 1;
			newIdst->parent = currIdst;
			newIdst->func_name = currIdst->func_name;

			if(currIdst->child == NULL)
				currIdst->child = newIdst;
			else
			{
				idsymboltable* helper = currIdst->child;
				while(helper->sibling != NULL)
				{
					helper = helper->sibling;
				}
				helper->sibling = newIdst;
			}

			stacknode* temp2 = curr->child->sibling->sibling->sibling->sibling->child;
			while(temp2 != NULL)
			{
				populateStatements(temp2, newIdst, globaltable, toPrint);
				temp2 = temp2->sibling;
			}
		}
		else
		{
			populateExpression(curr->child->sibling, currIdst, toPrint);
			idsymboltable* newIdst = makeidsymboltable();
			newIdst->startline = curr->child->sibling->sibling->tokinfo->linenumber;
			newIdst->endline = curr->child->sibling->sibling->sibling->sibling->tokinfo->linenumber;
			newIdst->nestinglevel = currIdst->nestinglevel + 1;
			newIdst->parent = currIdst;
			newIdst->func_name = currIdst->func_name;

			if(currIdst->child == NULL)
				currIdst->child = newIdst;
			else
			{
				idsymboltable* helper = currIdst->child;
				while(helper->sibling != NULL)
				{
					helper = helper->sibling;
				}
				helper->sibling = newIdst;
			}

			stacknode* temp = curr->child->sibling->sibling->sibling->child;
			while(temp != NULL)
			{
				populateStatements(temp, newIdst, globaltable, toPrint);
				temp = temp->sibling;
			}
		}
	}
	else if(strcmp(curr->ntortinfo->str, "<assignmentStmt>") == 0)
	{
		idsymboltable* temp = checkScope(currIdst, curr->child->tokinfo->lexeme);
		if(temp == NULL)
		{
			semanticCorrect = 1;
			if(toPrint){
				printf("ERROR_V at line %d : %s not declared in this scope.\n", curr->child->tokinfo->linenumber, curr->child->tokinfo->lexeme);
			}
		}
		else{
			curr->child->idst = temp;	// setting symbol table link for this ID
			idsymboltablenode* helper = getidsymboltablenode(curr->child->tokinfo->lexeme, curr->child->idst);
			helper->isAssigned = 1;
		}

		if(strcmp(curr->child->sibling->ntortinfo->str, "<lvalueIDStmt>") == 0)
			populateExpression(curr->child->sibling->child, currIdst, toPrint);
		else
		{
			if(strcmp(curr->child->sibling->child->child->ntortinfo->str, "ID") == 0)
			{
				idsymboltable* temp = checkScope(currIdst, curr->child->sibling->child->child->tokinfo->lexeme);
				if(temp == NULL)
				{	
					semanticCorrect = 1;
					if(toPrint){
						printf("ERROR_V at line %d : %s not declared in this scope.\n", curr->child->sibling->child->child->tokinfo->linenumber, curr->child->sibling->child->child->tokinfo->lexeme);
					}
				}
				else
					curr->child->sibling->child->child->idst = temp;	// setting symbol table link for this ID
			}
			populateExpression(curr->child->sibling->child->sibling, currIdst, toPrint);
		}
	}
	else if(strcmp(curr->ntortinfo->str, "<condionalStmt>") == 0)
	{
		idsymboltable* temp = checkScope(currIdst, curr->child->tokinfo->lexeme);
		if(temp == NULL)
		{
			semanticCorrect = 1;
			if(toPrint){
				printf("ERROR_V at line %d : %s not declared in this scope.\n", curr->child->tokinfo->linenumber, curr->child->tokinfo->lexeme);
			}
		}
		else
			curr->child->idst = temp;	// setting symbol table link for this ID

		idsymboltable* newIdst = makeidsymboltable();
		newIdst->startline = curr->child->sibling->tokinfo->linenumber;
		newIdst->endline = curr->child->sibling->sibling->sibling->sibling->tokinfo->linenumber;
		newIdst->nestinglevel = currIdst->nestinglevel + 1;
		newIdst->parent = currIdst;
		newIdst->func_name = currIdst->func_name;

		if(currIdst->child == NULL)
			currIdst->child = newIdst;
		else
		{
			idsymboltable* helper = currIdst->child;
			while(helper->sibling != NULL)
			{
				helper = helper->sibling;
			}
			helper->sibling = newIdst;
		}

		stacknode* temp2 = curr->child->sibling->sibling->child;	// pointing to <value>.nptr
		while(temp2 != NULL)
		{
			stacknode* temp3 = temp2->sibling->child; // temp2->sibling pointing to <statements>.nptr
			while(temp3 != NULL)
			{
				populateStatements(temp3, newIdst, globaltable, toPrint);
				temp3 = temp3->sibling;
			}
			temp2 = temp2->sibling->sibling;
		}

		if(curr->child->sibling->sibling->sibling->child != NULL)	// pointing to default's child
		{
			stacknode* temp = curr->child->sibling->sibling->sibling->child->child;	// pointing to statement's child
			while(temp != NULL)
			{
				populateStatements(temp, currIdst, globaltable, toPrint);
				temp = temp->sibling;
			}
		}
	}
	else if(strcmp(curr->ntortinfo->str, "<moduleReuseStmt>") == 0)
	{
		if(curr->child->child != NULL)		// pointing to idList's child
		{
			stacknode* temp = curr->child->child;	// pointing to ID
			while(temp != NULL)
			{
				idsymboltable* temp2 = checkScope(currIdst, temp->tokinfo->lexeme);
				if(temp2 == NULL)
				{
					semanticCorrect = 1;
					if(toPrint){
						printf("ERROR_V at line %d : %s not declared in this scope.\n", temp->tokinfo->linenumber, temp->tokinfo->lexeme);
					}
				}
				else
					temp->idst = temp2;		// setting symbol table link for this ID

				temp = temp->sibling;
			}
		}

		mainsymboltablenode* temp = presentmainsymboltable(globaltable, curr->child->sibling->tokinfo->lexeme);
		if(temp == NULL)	// function was neither declared nor defined
		{	
			semanticCorrect = 1;
			if(toPrint){
				printf("ERROR_M at line %d : MODULE %s used but never declared before this.\n", curr->child->sibling->tokinfo->linenumber, curr->child->sibling->tokinfo->lexeme);
			}
		}
		else
		{
			char* f1 = temp->func_name;
			char* f2 = currIdst->func_name;

			if(temp->isdefined && temp->isdeclared){
				semanticCorrect = 1;
				if(toPrint){
					printf("ERROR_M %s already defined before %s, no need to declare %s.\n",f1, f2, f1);
				}
			}
			
			if(!temp->isdefined){	// this would never run already done, for this temp would be already NULL
				if(!temp->isdeclared){
					semanticCorrect = 1;
					if(toPrint){
						printf("ERROR_M %s should be declared before %s, since defintion of %s is not present before %s.\n",f1, f2, f1, f2);
					}
				}
			}
		}
		stacknode* temp2 = curr->child->sibling->sibling->child;	// pointing to ID
		while(temp2 != NULL)
		{
			idsymboltable* temp3 = checkScope(currIdst, temp2->tokinfo->lexeme);
			if(temp3 == NULL){
				semanticCorrect = 1;
				if(toPrint){
					printf("ERROR_V at line %d : %s not declared in this scope\n", temp2->tokinfo->linenumber, temp2->tokinfo->lexeme);
				}
			}
			else
				temp2->idst = temp3;
			temp2 = temp2->sibling;
		}
	}
}

void populatemainsymboltable(stacknode* curr, stacknode* parent, mainsymboltable* globaltable, int toPrint)
{
	if(curr == NULL)
		return;
	populatemainsymboltable(curr->child, curr, globaltable, toPrint);

	if(strcmp(curr->ntortinfo->str, "<moduleDeclarations>") == 0)
	{
		stacknode* temp = curr->child;	// pointing to function name ID, if the ID is func name, not storing it's symbol table link as it will always be globaltable
		while(temp != NULL)
		{
			if(!presentmainsymboltable(globaltable, temp->tokinfo->lexeme)){
				mainsymboltablenode* temp2 = insertmainsymboltable(globaltable, temp->tokinfo->lexeme);
				temp2->isdeclared = 1;
			}
			else
			{
				semanticCorrect = 1;
				if(toPrint){
					printf("ERROR_M at line %d : %s is already declared. Function overloading is not allowed.\n", temp->tokinfo->linenumber, temp->tokinfo->lexeme);
				}
			}
				
			temp = temp->sibling;
		}
	}
	else if(strcmp(curr->ntortinfo->str, "<module>") == 0)
	{
		func_name = curr->child->tokinfo->lexeme;	// pointing to function name ID, if the ID is func name, not storing it's symbol table link as it will always be globaltable
		mainsymboltablenode* pt = presentmainsymboltable(globaltable, func_name);
		if(pt == NULL)	// it means this module was not declared earlier
			pt = insertmainsymboltable(globaltable, func_name);		// if not already declared insert into main symbol table
		else
		{
			if(pt->isdefined){
				semanticCorrect = 1;
				if(toPrint){
					printf("ERROR_M Function overloading is not allowed. %s already defined once.\n", func_name);
				}
				return;
			}
		}
		pt->isdefined = 1;
		pt->iplist = curr->child->sibling;	// pointing to input_plist
		pt->oplist = pt->iplist->sibling;	// pointing to output_plist
	}
	else if(strcmp(curr->ntortinfo->str, "<moduleDef>") == 0)
	{
		func_name = parent->child->tokinfo->lexeme;
		mainsymboltablenode* pt = presentmainsymboltable(globaltable, func_name);
		if(strcmp(func_name, "program") == 0)	// func_name would not be preset in case of "program"
		{
			pt->idst = makeidsymboltable();
			pt->idst->startline = curr->child->tokinfo->linenumber;
			pt->idst->endline = curr->child->sibling->sibling->tokinfo->linenumber;
			pt->idst->nestinglevel = 1;
			pt->idst->func_name = func_name;
		}
		stacknode* temp = curr->child->sibling->child;	// pointing to <ioStmt> (and similar)
		while(temp != NULL)
		{
			populateStatements(temp, pt->idst, globaltable, toPrint);		
			temp = temp->sibling;
		}
	}
	else if(strcmp(curr->ntortinfo->str, "<driverModule>") == 0)
	{
		insertmainsymboltable(globaltable, "program");	// inserting "program"
	}
	else if(strcmp(curr->ntortinfo->str, "<input_plist>") == 0)
	{
		func_name = parent->child->tokinfo->lexeme;
		mainsymboltablenode* pt = presentmainsymboltable(globaltable, func_name);
		pt->idst = makeidsymboltable();
		pt->idst->startline = curr->sibling->sibling->child->tokinfo->linenumber;
		pt->idst->endline = curr->sibling->sibling->child->sibling->sibling->tokinfo->linenumber;
		pt->idst->nestinglevel = 1;
		pt->idst->func_name = func_name;

		stacknode* temp = curr->child;	// pointing to ID
		while(temp != NULL)
		{
			if(inSameScope(pt->idst, temp->tokinfo->lexeme))
			{
				semanticCorrect = 1;
				if(toPrint){
					printf("ERROR_V at line %d : %s already declared in this scope\n", temp->tokinfo->linenumber, temp->tokinfo->lexeme);
				}
			}
			else
			{
				idsymboltablenode* idstnode = insertidsymboltablenode(temp->tokinfo->lexeme, temp->child, pt->idst->offset, pt->idst);	// offset not considered
				idstnode->widthofid = getlengthofid(idstnode->type);
				pt->idst->offset += idstnode->widthofid;
				temp->idst = pt->idst;	// setting symbol table link for ID
			}
			temp = temp->sibling;
		}
	}
	else if(strcmp(curr->ntortinfo->str, "<output_plist>") == 0)
	{
		func_name = parent->child->tokinfo->lexeme;
		mainsymboltablenode* pt = presentmainsymboltable(globaltable, func_name);
		stacknode* temp = curr->child;	// pointing to ID
		while(temp != NULL)
		{
			if(inSameScope(pt->idst, temp->tokinfo->lexeme))
			{
				semanticCorrect = 1;
				if(toPrint){
					printf("ERROR_V %s already declared in this scope\n", temp->tokinfo->lexeme);
				}
			}
			else
			{
				idsymboltablenode* idstnode = insertidsymboltablenode(temp->tokinfo->lexeme, temp->child, pt->idst->offset, pt->idst);	// offset not considered
				idstnode->widthofid = getlengthofid(idstnode->type);
				pt->idst->offset += idstnode->widthofid;
				temp->idst = pt->idst;	// setting symbol table link for ID
			}
			temp = temp->sibling;
		}
	}
	
	if(curr->child == NULL)
		return;
	
	stacknode* temp = curr->child->sibling;
	while(temp != NULL)
	{
		populatemainsymboltable(temp, curr, globaltable, toPrint);
		temp = temp->sibling;
	}

	return;
}

mainsymboltablenode* presentmainsymboltable(mainsymboltable* table, char* func_name)
{
	int h = hash(func_name);
	mainsymboltablenode* pt = table->buckets[h];

	while(pt != NULL)
	{
		if(strcmp(func_name,pt->func_name) == 0)
			return pt;
		pt = pt->next;
	}
	return NULL;
}

mainsymboltablenode* insertmainsymboltable(mainsymboltable* table, char* func_name)
{
	int h = hash(func_name);
	mainsymboltablenode* pt = table->buckets[h];
	mainsymboltablenode* nd = makemainsymboltablenode(func_name);
	nd->next = pt;
	table->buckets[h] = nd;
	return nd;
}

mainsymboltablenode* makemainsymboltablenode(char* func_name)
{
	mainsymboltablenode* pt = (mainsymboltablenode*)malloc(sizeof(mainsymboltablenode));
	pt->func_name = func_name;
	pt->next = NULL;
	pt->idst = NULL;
	pt->isdefined = pt->isused = pt->isdeclared = 0;
	pt->iplist = pt->oplist = NULL;
}

void printmainsymboltable(mainsymboltable* globaltable)
{
	for(int i = 0;i < mainsymboltablesize; i++)
	{
		mainsymboltablenode* pt = globaltable->buckets[i];
		while(pt != NULL)
		{
			idsymboltable* temp = pt->idst;
			printFunctionTable(temp);
			pt = pt->next;	
		}
	}
}

mainsymboltable* makemainsymboltable()
{
	mainsymboltable* pt = (mainsymboltable*) malloc(sizeof(mainsymboltable));
	for(int i=0; i<mainsymboltablesize; i++)
		pt->buckets[i] = NULL;
}