// xbly.c converts from XBLY on stdin to XML on stdout
// COMPILE: cc -std=c99 -Wall -Wextra xbly.c -o xbly
// USAGE: echo "(foo (bar))" | ./xbly
// EXIT CODE: zero on success

// SPDX-License-Identifier: MIT-0

/*

 XBLY.c

 Copyright © 2020 Tawesoft Ltd <open-source@tawesoft.co.uk>
 Copyright © 2020 Ben Golightly <ben@tawesoft.co.uk>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction,  including without limitation the rights
 to use,  copy, modify,  merge,  publish, distribute, sublicense,  and/or sell
 copies  of  the  Software,  and  to  permit persons  to whom  the Software is
 furnished to do so.

 THE SOFTWARE IS PROVIDED  "AS IS",  WITHOUT WARRANTY OF ANY KIND,  EXPRESS OR
 IMPLIED,  INCLUDING  BUT  NOT LIMITED TO THE WARRANTIES  OF  MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE  AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 AUTHORS  OR COPYRIGHT HOLDERS  BE LIABLE  FOR ANY  CLAIM,  DAMAGES  OR  OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

*/

#include <ctype.h> // isspace
#include <stdio.h> // fgetc, fputc, etc.
#include <stdlib.h> // exit
#include <limits.h> // CHAR_MAX

#define whitespace(x) (isspace(x) != 0)

// Parser states. Illustrated by example: consider the string:
//     `<?xml version="1.0)?>(parent \enabled \fruit="apple" (child some text \(hi!\)))`
enum estate {
    ParsingStart,          // <?xml version="1.0)?>
    ParsingElementName,    // `parent` and `child`
    ParsingAttributes,     // `\enabled \fruit="apple"`
    ParsingAttribute,      // `\enabled` and `\fruit="apple"`
    ParsingAttributeValue, // `"apple"`
    ParsingText,           // `some text \(hi!\)`
    ParsingTextEscaped,    // `\(` and `\)` in `some text \(hi!\)`
};
enum estate state = ParsingStart;

// A stack of element names. Each name is terminated by a single byte
// indicating its length.
#define NAMESTACKZ (16*1024)
char nameStack[NAMESTACKZ];
int nameStackTop = 0;
int nameLen = 0;

// push a character or length on to the stack
void push(int a) {
    if (nameLen >= CHAR_MAX) {
        fprintf(stderr, "element name too long\n");
        exit(1);
    }
    if (nameStackTop >= NAMESTACKZ) {
        fprintf(stderr, "stack overflow\n");
        exit(1);
    }

    nameLen++;
    nameStack[nameStackTop] = (char) a;
    nameStackTop++;
}

// pop and print a name from the stack
void pop_name() {
    if (nameStackTop <= 0) {
        fprintf(stderr, "stack underflow\n");
        exit(1);
    }

    fputc('<', stdout);
    fputc('/', stdout);

    int len = (int) nameStack[nameStackTop - 1];
    for (int i = nameStackTop - len - 1; i < nameStackTop - 1; i++) {
        fputc(nameStack[i], stdout);
    }

    nameStackTop = nameStackTop - len - 1;

    fputc('>', stdout);
}

// discard the top name from the stack
void discard_name() {
    if (nameStackTop <= 0) {
        fprintf(stderr, "stack underflow\n");
        exit(1);
    }
    int len = (int) nameStack[nameStackTop - 1];
    nameStackTop = nameStackTop - len - 1;
}

void start_element_name() {
    fputc('<', stdout);
    state = ParsingElementName;
    nameLen = 0;
}

int main(void) {
    int current = EOF;
    int next = fgetc(stdin);
    int seenQuote = 0; // zero, ' or "

    while (next != EOF) {
        current = next;
        next = fgetc(stdin);

        switch (state) {
            case ParsingStart:
                // Anything up to the first opening parenthesis is copied
                // verbatim.

                if (current == '(') {
                    start_element_name();
                } else {
                    fputc(current, stdout);
                }
                break;

            case ParsingElementName:
                // Copy the element name, storing its name on the stack until:
                //
                // - whitespace marks the end of the element name and attribute
                //   processing starts (attributes may be empty) e.g. `(p foo)`
                //   or `(br )` or `(p \class="foo" ...`.
                //
                // - a closing parenthesis immediately closes the element e.g.
                //   `(br)`, at which point the element is an Empty-element Tag
                //  and can be closed immediately as e.g. `<br/>`.

                if (whitespace(current)) {
                    fputc(current, stdout);
                    push(nameLen);
                    state = ParsingAttributes;
                } else if (current == ')') {
                    push(nameLen);
                    discard_name();
                    fputs(" />", stdout);
                    state = ParsingText;
                } else {
                    fputc(current, stdout);
                    push(current);
                }
                break;

            case ParsingAttributes:
                // Parse attributes starting with backslash plus any character
                // other than backslash, open or close parenthesis.
                // e.g. (p \class="foo" \class="bar" ...)
                //
                // Or, open a new element e.g. `(a \class="foo" (b ...))`
                //                              ----------------^
                //
                // Or, close close the current element e.g. `(a \class="foo")`
                //                                           ---------------^
                // immediately without finding any text or child elements
                //
                // Or, find some text content e.g. `(a \class="foo" Hello)`
                //                                  ----------------^
                if (current == '(') {
                    fputc('>', stdout);
                    start_element_name();
                } else if (current == ')') {
                    discard_name();
                    fputs(" />", stdout);
                    state = ParsingText;
                } else if (whitespace(current)) {
                    fputc(current, stdout);
                } else if (current == '\\') {
                    if ((next == '\\') || (next == '(') || (next == ')')) {
                        fputc('>', stdout);
                        fputc(current, stdout);
                        state = ParsingText;
                    } else if (whitespace(next)) {
                        fputc('>', stdout);
                        state = ParsingText;
                    } else {
                        state = ParsingAttribute;
                    }
                } else {
                    fputc('>', stdout);
                    fputc(current, stdout);
                    state = ParsingText;
                }
                break;

            case ParsingAttribute:
                // Parse a single attribute e.g. `\foo="bar"`, returning to
                // attributes parsing mode on completion.
                //
                // Single or double quotes are fine.

                fputc(current, stdout);
                if (current == '=') {
                    state = ParsingAttributeValue;
                    seenQuote = 0;
                } else if (current == ' ') {
                    // tolerate invalid XML such as <foo enabled>
                    state = ParsingAttributes;
                }
                break;

            case ParsingAttributeValue:
                // Parse a single attribute value e.g. "foo"
                fputc(current, stdout);

                if (seenQuote) {
                    if (current == seenQuote) {
                        // matching end quote, so we're done
                        state = ParsingAttributes;
                    }
                } else {
                    if ((current == '"') || (current == '\'')) {
                        // found the starting quote
                        seenQuote = current;
                    }
                }
                break;

            case ParsingText:
                // Parse text content
                // - starting a new element on open parenthesis
                // - closing the current element on close parenthesis,
                // - escaping the next character on backslash

                if (current == '(') {
                    start_element_name();
                } else if (current == ')') {
                    pop_name();
                } else if (current == '\\') {
                    state = ParsingTextEscaped;
                } else {
                    fputc(current, stdout);
                }
                break;

            case ParsingTextEscaped:
                if ((current == '(') || (current == ')') || (current == '\\') || (whitespace(current))) {
                    fputc(current, stdout);
                    state = ParsingText;
                } else {
                    fprintf(stderr, "illegal escape sequence\n");
                    return -1;
                }
                break;
        }
    }

    if (nameStackTop) {
        fprintf(stderr, "unexpected EOF\n");
        return -1;
    }

    return ferror(stdin) ? ferror(stdin) : ferror(stdout);
}
