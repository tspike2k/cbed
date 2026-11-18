//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

/*
TODO:
    enums
*/


#include "common.c"
#include "files.c"

u8 g_memory[6*1024*1024];

typedef enum {
    Token_Type_None,
    Token_Type_Unknown,
    Token_Type_End_Of_File,
    Token_Type_Identifier,
    Token_Type_Typedef,
    Token_Type_Struct,
    Token_Type_Line_Comment,
    Token_Type_Multi_Line_Comment,
    Token_Type_Pound,
    Token_Type_String,
    Token_Type_Greater_Than,
    Token_Type_Less_Than,
    Token_Type_Slash,
    Token_Type_API,
    Token_Type_Open_Paren,
    Token_Type_Close_Paren,
    Token_Type_Comma,
    Token_Type_Void,
} Token_Type;

typedef struct{
    String source;
    u32    line;
} Tokenizer;

typedef struct{
    Token_Type type;
    String     text;
    u32        line;
} Token;

static Tokenizer make_tokenizer(String source){
    Tokenizer result = {0};
    result.source = source;
    result.line = 1;
    return result;
}

static bool char_is_alpha(char c){
    bool result = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    return result;
}

static char peek_char(Tokenizer *s){
    char result = '\0';
    if(s->source.size){
        result = s->source.text[0];
    }
    return result;
}

static char next_char(Tokenizer *s){
    char result = '\0';
    if(s->source.size){
        result = s->source.text[0];
        str_advance(&s->source);
    }
    return result;
}

static Token_Type check_keyword(String s, String keyword, Token_Type keyword_token_type){
    Token_Type result = Token_Type_Identifier;
    if(s.size == keyword.size && memcmp(s.text, keyword.text, s.size) == 0){
        result = keyword_token_type;
    }

    return result;
}

static Token_Type parse_keyword_or_identifier(String s){
    assert(s.size);

    Token_Type result;
    switch(s.text[0]){
        default:
            result = Token_Type_Identifier; break;

        case 'C': result = check_keyword(s, str_lit("Cbed_API"), Token_Type_API); break;
        case 'v': result = check_keyword(s, str_lit("void"),     Token_Type_Void); break;
    }

    return result;
}

static Token next_token(Tokenizer *s){
    String *reader = &s->source;
    if(reader->size <= 0){
        Token result = {Token_Type_End_Of_File};
        return result;
    }

    // Skip whitespace
    while(reader->size){
        char c = reader->text[0];
        if(c == '\n'){
            s->line++;
            str_advance(reader);
        }
        else if(char_is_whitespace(c)){
            str_advance(reader);
        }
        else
            break;
    }

    Token result = {Token_Type_Unknown};
    result.text.text = reader->text;
    result.line = s->line;

    char c = next_char(s);
    switch(c){
        case '/':{
            char c1 = peek_char(s);
            if(c1 == '/'){
                //
                // Consume single-line comment
                //
                str_advance(reader); // Consume second / char

                result.type = Token_Type_Line_Comment;
                result.text = *reader;
                result.line = s->line;
                while(reader->size && reader->text[0] != '\n'){
                    str_advance(reader);
                }
                result.text.size = reader->text - result.text.text;

            }
            else if(c1 == '*'){
                //
                // Consume multi-line comment
                //
                result.type = Token_Type_Multi_Line_Comment;
                result.text = *reader;
                result.line = s->line;
                char prev_c = '\0';
                while(reader->size){
                    if(reader->text[0] == '/' && prev_c == '*'){
                        result.text.size = reader->text - result.text.text;
                        str_advance(reader);
                        str_advance(reader);
                        break;
                    }

                    prev_c = reader->text[0];
                    str_advance(reader);
                }
            }
            else{
                result.type = Token_Type_Slash;
                result.text.size = 1;
            }
        } break;

        case '"':{
            result.type = Token_Type_String;
            result.text = *reader;
            char prev_char = '\0';
            while(reader->size){
                if(reader->text[0] == '"' && prev_char != '\\'){
                    result.text.size = reader->text - result.text.text;
                    break;
                }

                prev_char = reader->text[0];
                str_advance(reader);
            }
            str_advance(reader); // Skip the closing quote
        } break;

        case '#':{
            result.type = Token_Type_Pound;
            result.text.size = 1;
        } break;

        case '<':{
            result.type = Token_Type_Less_Than;
            result.text.size = 1;
        } break;

        case '>':{
            result.type = Token_Type_Greater_Than;
            result.text.size = 1;
        } break;

        case ',':{
            result.type = Token_Type_Comma;
            result.text.size = 1;
        } break;

        case '(':{
            result.type = Token_Type_Open_Paren;
            result.text.size = 1;
        } break;

        case ')':{
            result.type = Token_Type_Close_Paren;
            result.text.size = 1;
        } break;

        default:{
            if(char_is_alpha(c)){
                // TODO: Allow numbers in identifiers!
                while(reader->size && (char_is_alpha(reader->text[0]) || reader->text[0] == '_')){
                    str_advance(reader);
                }
                result.text.size = reader->text - result.text.text;
                result.type = parse_keyword_or_identifier(result.text);
            }
        } break;
    }

    return result;
}

static Token peek_token(Tokenizer *s){
    Tokenizer restore = *s;
    Token result = next_token(s);
    *s = restore;
    return result;
}

static bool should_add_extra_newline(Token prev_token, Token token){
    bool result = token.line > 1 && (prev_token.type != Token_Type_Line_Comment || token.line - prev_token.line > 1);
    return result;
}

static String trim_extension(String s){
    String result = s;
    for(u32 i = s.size; i > 0; i--){
        if(s.text[i-1] == '.'){
            result.size = i-1;
            break;
        }
    }
    return result;
}

typedef enum{
    Parse_Type_None,
    Parse_Type_Comment,
    Parse_Type_Include,
    Parse_Type_Function,
    Parse_Type_Define,
} Parse_Type;

typedef struct{
    Parse_Type type;
    String     source;
    u32        line;
} Parse_Comment;

typedef struct{
    Parse_Type type;
    String file_name;
    bool   is_system_file;
} Parse_Include;

typedef struct Var_Decl Var_Decl;

struct Var_Decl{
    Var_Decl *next;
    String    name;
    String    type_name;
    String    comment; // Used for comments like these in struct declarations
};

typedef struct{
    Parse_Type type;
    String     name;
    String     return_type;
    Var_Decl   parameters;
} Parse_Function;

typedef struct{
    Parse_Type type;
    String name;
    String params;
    String value;
} Parse_Define;

typedef struct Parse_Node Parse_Node;

struct Parse_Node{
    Parse_Node *next;
    union{
        Parse_Type     type;
        Parse_Comment  comment;
        Parse_Include  include;
        Parse_Function function;
    };
};

typedef struct{
    Parse_Node *first;
    Parse_Node *last;
} Parse_Nodes;

static Parse_Node *add_parse_node(Parse_Nodes *nodes, u32 node_type, Buffer *memory){
    Parse_Node *result = buffer_push_type(Parse_Node, memory);
    result->type = node_type;

    if(!nodes->first){
        nodes->first = result;
    }
    else if(nodes->last){
        nodes->last->next = result;
    }

    nodes->last = result;
    return result;
}

Parse_Nodes parse_file(String source, Buffer *memory){
    Parse_Nodes nodes = {0};

    Token prev_token = {0};
    Tokenizer tokenizer = make_tokenizer(source);
    while(peek_token(&tokenizer).type != Token_Type_End_Of_File){
        Token token = next_token(&tokenizer);

        switch(token.type){
            default: break;

            case Token_Type_Pound:{
                Token peek = peek_token(&tokenizer);
                if(str_match(peek.text, str("include"))){
                    next_token(&tokenizer); // Skip "include" token
                    Parse_Node *node = add_parse_node(&nodes, Parse_Type_Include, memory);
                    Parse_Include *include = &node->include;

                    token = next_token(&tokenizer);
                    if(token.type == Token_Type_String){
                        include->file_name = token.text;
                        include->is_system_file = false;
                    }
                    else if(token.type == Token_Type_Less_Than){
                        String file_name = tokenizer.source;
                        while(tokenizer.source.size){
                            if(peek_token(&tokenizer).type == Token_Type_Greater_Than){
                                file_name.size = tokenizer.source.text - file_name.text;
                                break;
                            }
                            next_token(&tokenizer);
                        }

                        include->file_name = file_name;
                        include->is_system_file = true;
                    }
                }
                else{
                    // Ignore this macro. Skip until the next line
                    // TODO: What about mulit-line macros? They end escape line endings with
                    // a slash. How should that work here? Maybe that's a special token?
                    u32 line = token.line;
                    while(peek_token(&tokenizer).line == line){
                        next_token(&tokenizer);
                    }
                }
            } break;

            case Token_Type_API:{
                Parse_Node *node = add_parse_node(&nodes, Parse_Type_Function, memory);
                Parse_Function *func = &node->function;

                func->return_type = next_token(&tokenizer).text;
                func->name        = next_token(&tokenizer).text;
                assert(next_token(&tokenizer).type == Token_Type_Open_Paren); // TODO: Handle errors

                Var_Decl *param = &func->parameters;
                Token peek = peek_token(&tokenizer);
                if(peek.type == Token_Type_Void){
                    param->type_name = str_lit("void");
                    next_token(&tokenizer);
                }
                else{
                    while(peek.type != Token_Type_End_Of_File){
                        param->type_name = next_token(&tokenizer).text;
                        param->name      = next_token(&tokenizer).text;

                        peek = peek_token(&tokenizer);
                        if(peek.type == Token_Type_Comma){
                            next_token(&tokenizer);
                        }
                        else if(peek.type == Token_Type_Close_Paren){
                            break;
                        }
                        else{
                            assert(0); // TODO: Handle errors.
                        }
                    }
                }

                assert(peek_token(&tokenizer).type == Token_Type_Close_Paren); // TODO: Handle errors.
                next_token(&tokenizer);
            } break;

            case Token_Type_Line_Comment:{
                /*if(should_add_extra_newline(prev_token, token))*/
                    /*printf("\n");*/

                /*printf("//%.*s\n", (int)token.text.size, token.text.text);*/
            } break;

            case Token_Type_Multi_Line_Comment:{
                /*if(should_add_extra_newline(prev_token, token))*/
                    /*printf("\n");*/

                //printf("/*%.*s*/\n", (int)token.text.size, token.text.text);
            } break;
        }

        prev_token = token;
    }

    return nodes;
}

static String str_copy(String s, Buffer *memory){
    char *dest = buffer_push_bytes(memory, s.size+1); // Null-terminate, just in case.
    memcpy(dest, s.text, s.size);
    dest[s.size] = '\0';
    String result = {dest, s.size};
    return result;
}

int main(){
    Buffer memory = {&g_memory[0], Array_Len(g_memory)};

    /*String source = file_read_into_memory("./src/atlas.h", &memory);*/
    String source = file_read_into_memory("./test.h", &memory);
    Parse_Nodes parse_nodes = parse_file(source, &memory);
    Parse_Node *prev_node = NULL;
    for(Parse_Node *node = parse_nodes.first; node; node = node->next){
        switch(node->type){
            default: break;

            case Parse_Type_Include:{
                Buffer memory_restore = memory;

                Parse_Include *include = &node->include;
                String name = str_copy(include->file_name, &memory);
                name = trim_extension(name);
                if(!str_match(name, str("stdint")) && !str_match(name, str("stdbool"))){
                    for_count(u32, i, name.size){
                        if(name.text[i] == '/' || name.text[i] == '\\'){
                            name.text[i] = '.';
                        }
                    }

                    if(!include->is_system_file){
                        printf("public import %.*s;\n", (int)name.size, name.text);
                    }
                    else{
                        printf("public import core.stdc.%.*s;\n", (int)name.size, name.text);
                    }
                }

                memory = memory_restore;
            } break;

            case Parse_Type_Function:{
                Parse_Function *func = &node->function;

                printf(
                    "extern(C) %.*s %.*s(",
                    (int)func->return_type.size, func->return_type.text,
                    (int)func->name.size, func->name.text
                );

                printf(")\n");
            } break;
        }
    }

    return 0;
}
