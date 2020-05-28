#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <assert.h>

#include "textbuffer.h"

#define BUFFER_SIZE 1000

typedef struct line *Line;

struct textbuffer {
	Line first;
	Line last;
	int size;
};

struct line {
	struct line *next;
	int lNum;
	char *text;
};

// Mallocs a newLine, initialising some struct attributes, and 
// mallocing string of 'size' for its text. Returns the line.
static Line newLine(int size);
// Frees a linked list of lines recursively. 
// (Last Line)->next == NULL
static void freeLineRec(Line l);
// Loops through tb, amending line number problems by other 
// functions, and fixing tb->size
static void fixLineNums(TB tb);
// Returns a pointer to newly allocated Match node, initialising
// values
static Match newMatch(void);
// Asserts some attributes that make a TB valid. Diagnostic. 
static void validTB(TB tb);
// Implements hash richText replacement. Returns a pointer to the
// new malloced line
static char *hashRich(char *text);
// Implements asterisk richText replacement. Returns a pointer to the
// new malloced line
static char *uScoreAndAstRich(char *text);


/**
 * Allocate a new textbuffer whose contents is initialised with the text
 * in the given string.
 */
TB newTB (char *text) {
	// NULL text
	if (text == NULL) {
		printf("NULL string is invalid");
		abort();
	}
	
	TB new = malloc(sizeof(struct textbuffer));
	*new = (struct textbuffer) {
		.first = NULL,
		.last = NULL,
		.size = 0
	};

	// Empty text, return empty struct
	if (strlen(text) == 0) return new;
	
	// nextBreak records address of next \n. prevBreak records previous.
	char *nextBreak = strstr(text, "\n"), *prevBreak = &text[0];
	// size is difference between these two (plus '\0')
	int size = (int) (nextBreak - prevBreak) + 1;
	Line curr = newLine(size);
	
	new->first = new->last = curr;

	// inLine records position in curr->text, i in big text
	// Adds letter by letter. Keeps /n
	int i = 0, inLine = 0;
	while (text[i] != '\0') {
		curr->text[inLine] = text[i];
		inLine++;
		// \n, so new Line (unless this is last \n)
		if (text[i] == '\n' && text[i + 1] != '\0') {
			curr->text[inLine] = '\0';
			// New pointers and size
			prevBreak = nextBreak;
			nextBreak = strstr(&text[i + 1], "\n");
			size = (int) (nextBreak - prevBreak) + 1;
			curr = newLine(size);
			// Linking in new curr:
			new->last->next = curr;
			new->last = curr;
			new->size++;
			inLine = 0;
		}

		i++;
	}
	curr->text[inLine] = '\0';
	fixLineNums(new);
	validTB(new);
	return new;
}

/**
 * Free  the  memory occupied by the given textbuffer. It is an error to
 * access the buffer afterwards.
 */
void releaseTB (TB tb) {
	if (tb->size != 0) {
		freeLineRec(tb->first);
	}
	free(tb);
}

/**
 * Allocate  and return a string containing all of the text in the given
 * textbuffer. If showLineNumbers is true, add a prefix corresponding to
 * the line number.
 */
char *dumpTB (TB tb, bool showLineNumbers) {
	// Empty TB so empty string returned
	if (tb->size == 0) {
		char *empty = malloc(sizeof(""));
		strcpy(empty, "");
		return empty;
	}

	// Get size of all text
	int TBSize = 0;
	for (Line curr = tb->first; curr != NULL; curr = curr->next) {
		TBSize+=(strlen(curr->text));
		if (showLineNumbers) TBSize+=(sizeof(int) + strlen(". "));
	}

	// String containing all text
	char *full = malloc(TBSize + 1);
	// Since everything is added with strcat
	full[0] = '\0';
	Line curr = tb->first;
	while (curr != NULL) {
		if (showLineNumbers) {
			// str to hold line number and '. '
			char str[sizeof(int) + strlen(". ")];
			sprintf(str, "%d", curr->lNum);
			strcat(str, ". ");
			strcat(full, str);
		}
		strcat(full, curr->text);
		curr = curr->next;
	}
	return full;
}

/**
 * Return the number of lines of the given textbuffer.
 */
int linesTB (TB tb) {
	return tb->size;
}

/**
 * Add a given prefix to all lines between 'from' and 'to', inclusive.
 * - The  program  should abort() wih an error message if 'from' or 'to'
 *   is out of range. The first line of a textbuffer is at position 1.
 */
void addPrefixTB (TB tb, int from, int to, char *prefix) {
	// Error checking
	if (from > to || to > tb->size) {
		printf("'From' or 'to' out of range\n");
		abort();
	}

	// Empty prefix, no changes
	if (strcmp(prefix, "") == 0) return;

	// Loop until 'from'
	Line l = tb->first;
	while (l->lNum < from) {
		l = l->next;
	}

	while (l->lNum <= to) {
		// New text with room for prefix
		char *text = malloc(strlen(l->text) + strlen(prefix) + 1);
		strcpy(text, prefix);
		strcat(text, l->text);
		// free old text
		free(l->text);
		l->text = text;
		l = l->next;
	}
}

/**
 * Merge 'tb2' into 'tb1' at line 'pos'.
 * - After this operation:
 *   - What was at line 1 of 'tb2' will now be at line 'pos' of 'tb1'.
 *   - Line  'pos' of 'tb1' will moved to line ('pos' + linesTB('tb2')),
 *     after the merged-in lines from 'tb2'.
 *   - 'tb2' can't be used anymore (as if we had used releaseTB() on it)
 * - The program should abort() with an error message if 'pos' is out of
 *   range.
 */
void mergeTB (TB tb1, int pos, TB tb2) {
	// Error checking
	if (tb1->size + 1 < pos) {
		printf("Insert position outside range of textbuffer\n");
		abort();
	}

	// Trying to merge tb1 into itself
	if (tb1->first == tb2->first && tb1->first != NULL) return;

	// Empty first list means tb1 becomes tb2
	if (tb1->first == NULL) {
		if (pos != 1) {
			printf("Invalid pos value\n");
			abort();
		}
		*tb1 = (struct textbuffer) {
			.first = tb2->first,
			.last = tb2->last,
			.size = tb2->size,
		};
		*tb2 = (struct textbuffer) {
			.first = NULL,
			.last = NULL,
			.size = 0,
		};
		releaseTB(tb2);
		return;
	}

	// Traverse to pos
	Line curr = tb1->first;
	while (curr->lNum < pos - 1) {
		curr = curr->next;
	}

	// Linking tb2 into tb1:
	if (pos == 1) {
		// If replacing first
		tb2->last->next = tb1->first;
		tb1->first = tb2->first;
	} else if (tb1->last->lNum == pos - 1) {
		// Appending to end
		tb1->last->next = tb2->first;
		tb1->last = tb2->last;
	} else {
		tb2->last->next = curr->next;
		curr->next = tb2->first;
	}

	fixLineNums(tb1);

	free(tb2);
}
 
/**
 * Copy 'tb2' into 'tb1' at line 'pos'.
 * - After this operation:
 *   - What was at line 1 of 'tb2' will now be at line 'pos' of 'tb1'.
 *   - Line  'pos' of 'tb1' will moved to line ('pos' + linesTB('tb2')),
 *     after the pasted-in lines from 'tb2'.
 *   - 'tb2' is unmodified and remains usable independent of tb1.
 * - The program should abort() with an error message if 'pos' is out of
 *   range.
 */
void pasteTB (TB tb1, int pos, TB tb2) {
	// Get all text from tb2 then send it to newTB as if it were new.
	// merge in this new TB.

	if (tb1->size + 1 < pos) {
		printf("Insert position outside range of textbuffer\n");
		abort();
	}

	// Empty tb2, nothing to do 
	if (tb2->size == 0) return;

	// Empty tb1 is dealt with as usual

	// Copy all text in tb2
	Line l = tb2->first;
	int space = 0;
	while (l != NULL) {
		space += strlen(l->text) + 1;
		l = l->next;
	}

	// into a text dump
	char bigText[space];
	bigText[0] = '\0';
	l = tb2->first;
	while (l != NULL) {
		strcat(bigText, l->text);
		l = l->next;
	}
	
	// Use newTB() to make a new TB from that text
	TB toInsert = newTB(bigText);

	// Then merge into tb1 at pos
	mergeTB(tb1, pos, toInsert);
}

/**
 * Cut  the lines between and including 'from' and 'to' out of the given
 * textbuffer 'tb' into a new textbuffer.
 * - The result is a new textbuffer (much as one created with newTB()).
 * - The cut lines will be deleted from 'tb'.
 * - The  program should abort() with an error message if 'from' or 'to'
 *   is out of range.
 */
TB cutTB (TB tb, int from, int to) {
	// Error checking
	if (tb->size < to || from < 1) {
		printf("Cut position outside range of textbuffer\n");
		// PROBLEM Not sure which. Spec says both
		abort();
		return NULL;
	}

	// Get pointers to 'from', 'to' and 'beforeFrom'
	Line toLine = tb->first, fromLine = NULL, beforeFrom = NULL;
	while (toLine->lNum < to) {
		if (toLine->lNum == from - 1) {
			fromLine = toLine->next;
			beforeFrom = toLine;
		}
		toLine = toLine->next;
	}

	// Cut out 'from' to 'to' from tb. Rejoin original
	if (from == 1) {
		// Removing first
		fromLine = tb->first;
		tb->first = toLine->next;
	} 

	if (to == tb->last->lNum) {
		// Deleting end
		tb->last = beforeFrom;
		if (tb->last != NULL) tb->last->next = NULL;
	} 

	// If !(front of list), else accessing NULL
	if (beforeFrom != NULL) beforeFrom->next = toLine->next;


	TB new = malloc(sizeof(struct textbuffer));
	new->first = fromLine;
	new->last = toLine;
	// Fully remove it from tb
	toLine->next = NULL;

	fixLineNums(tb);
	fixLineNums(new);

	return new;
}

/**
 * Return  a  linked list of match nodes containing the positions of all
 * of the matches of string 'search' in 'tb'.
 * - The textbuffer 'tb' should remain unmodified.
 * - The user is responsible for freeing the returned list.
 */
Match searchTB (TB tb, char *search) {
	// Empty search
	if (strlen(search) == 0) return NULL;


	Match firstMatch = newMatch(), currMatch = firstMatch;
	
	for (Line currLine = tb->first; currLine != NULL; currLine = currLine->next)
	{
		// First occurrence of the search term in currLine->text
		char *firstOcc = strstr(currLine->text, search);
		long index = 0;
		// Not sure about second condition for 
		while (firstOcc != NULL && (int) index < strlen(currLine->text)) {
			// position of match in currLine->text
			index = firstOcc - currLine->text;
			// If not the first match
			if (currMatch->columnNumber != 0) {
				currMatch->next = newMatch();
				currMatch = currMatch->next;
			}
			// 1 indexed
			currMatch->columnNumber = (int) index + 1; 
			currMatch->lineNumber = currLine->lNum;
			// next occurrence, searching the rest of the line
			firstOcc = strstr(&currLine->text[strlen(search) + index], search);
		}
	}

	// No matches, free the first match
	if (firstMatch->lineNumber == 0) {
		free(firstMatch);
		return NULL;
	}
	return firstMatch;
}


/**
 * Remove  the  lines between 'from' and 'to' (inclusive) from the given
 * textbuffer 'tb'.
 * - The  program should abort() with an error message if 'from' or 'to'
 *   is out of range.
 */
void deleteTB (TB tb, int from, int to) {
	// Error checking
	if (tb->size < to || from < 1) {
		printf("Delete position outside range of textbuffer\n");
		abort();
	}

	// Like in cutTB
	Line toLine = tb->first, fromLine = NULL, beforeFrom = NULL;
	// Loop until 'to' node, recording 'from' along the way
	while (toLine->lNum < to) {
		if (toLine->lNum == from - 1) {
			fromLine = toLine->next;
			beforeFrom = toLine;
		}
		toLine = toLine->next;
	}
	// Cut out 'from' to 'to' from tb. Rejoin original
	if (from == 1) {
		// Removing first
		fromLine = tb->first;
		tb->first = toLine->next;
	} 

	if (to == tb->last->lNum) {
		// Deleting end
		tb->last = beforeFrom;
		if (tb->last != NULL) tb->last->next = NULL;
	} 

	// If !(front of list), else accessing NULL
	if (beforeFrom != NULL) beforeFrom->next = toLine->next;
	
	fixLineNums(tb);

	// Free the cut out list recursively
	toLine->next = NULL;
	freeLineRec(fromLine);
}

/**
 * Search  every  line of the given textbuffer for every occurrence of a
 * set of specified substitutions and alter them accordingly.
 * - Refer to the spec for details.
 */
void formRichText (TB tb) {
	// Every line 
	for (Line l = tb->first; l != NULL; l = l->next) {
		char *updatedString;

		if (l->text[0] == '#') {
			updatedString = hashRich(l->text);
		} else {
			updatedString = uScoreAndAstRich(l->text);
		}

		free(l->text);
		l->text = updatedString;
	}
}


////////////////////////////////////////////////////////////////////////
// Bonus challenges

char *diffTB (TB tb1, TB tb2) {
	return NULL;
}

void undoTB (TB tb) {

}

void redoTB (TB tb) {

}

///////// My Helpers /////////

// Mallocs a struct line with that text
static Line newLine(int size) {
	
	Line newL = malloc(sizeof(struct line));
	newL->text = malloc(size + 1);
	newL->next = NULL;
	return newL;
}

// Frees a linked list of lines recursively. (End of list)->next
// must be NULL
static void freeLineRec(Line l) {
	if (l == NULL) return;
	freeLineRec(l->next);
	free(l->text);
	free(l);
}

// Loops through tb and changes line numbers and tb->size
static void fixLineNums(TB tb) {
	int i = 1;
	for (Line curr = tb->first; curr != NULL; curr = curr->next) {

		curr->lNum = i;
		i++;
	}
	tb->size = i - 1;
}

// Diagnostic - checks the TB is valid
static void validTB(TB tb) {
	assert(tb->last->next == NULL);
	int i = 0;
	Line curr = tb->first;

	while (curr != NULL) {
		i++;
		assert(curr->lNum == i);
		curr = curr->next;
	}
	assert(i == tb->size);
}

// Mallocs a new match node 
static Match newMatch(void) {
	Match new = malloc(sizeof(struct _matchNode));
	*new = (struct _matchNode) {
		.columnNumber = 0,
		.lineNumber = 0,
		.next = NULL
	};
	return new;
}

// Implements asterisk richText replacement. Returns a pointer to the
// new malloced line
static char *uScoreAndAstRich(char *text) {
	
	// Difficult to find number of substitutions expected, so declaring
	// string much larger than expected.
	char copy[BUFFER_SIZE];
	// bc strcating
	copy[0] = '\0';
	int i = 0;

	while (text[i] != '\0') {
		char next = text[i];
		// A string of 'next' for strstr
		char nextStr[2];
		nextStr[0] = next;
		nextStr[1] = '\0';
		// If its a special char and there is a corresponding special char later
		if ((next == '_' || next == '*') 
			&& (strstr(&text[i + 2], nextStr) != NULL))
		{
			// cheeky ternary
			strcat(copy, ((next == '_') ? "<i>" : "<b>"));
			int j = i + 1;
			// Adding until the closing special char
			while (text[j] != next) {
				strncat(copy, &text[j], 1);
				j++;
			}
			strcat(copy, ((next == '_') ? "</i>" : "</b>"));
			// Resume from the end of this change
			i = j + 1;
		} else {
			// Not a special char so add it
			strncat(copy, &text[i], 1);
			i++;
		}
	}

	char *newText = malloc(strlen(copy) + 1);
	strcpy(newText, copy);
	return newText;
}

// Implements hash richText replacement. Returns a pointer to the
// new malloced line
static char *hashRich(char *text) {
	// Difficult to find number of substitutions expected, so declaring
	// string much larger than expected.
	char copy[BUFFER_SIZE];
	copy[0] = '\0';
	// Simple case for # sub
	strcat(copy, "<h1>");
	// What's stored strictly between # and \n
	strncat(copy, &text[1], strlen(text) - 2);
	strcat(copy, "</h1>\n");

	char *newText = malloc(strlen(copy) + 1);
	strcpy(newText, copy);
	return newText;
}
