//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "common.c"
#include "files.c"

u8 g_memory[6*1024*1024];

typedef enum {
    Token_Type_None,
    Token_Type_End_Of_File,
    Token_Type_Identifier,
    Token_Type_Typedef,
    Token_Type_Struct,
    Token_Type_Line_Comment,
    Token_Type_Multi_Line_Comment,
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

static Tokenizer tokenizer_save(Tokenizer *s){
    Tokenizer result = *s;
    return result;
}

static void tokenizer_restore(Tokenizer *s, Tokenizer restore){
    *s = restore;
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

    Token result = {0};
    result.text.text = reader->text;
    result.line = s->line;

    char c  = next_char(s);
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
                    if(reader->text[0] == '*' && prev_c == '/'){
                        result.text.size = reader->text - result.text.text;
                        str_advance(reader);
                        str_advance(reader);
                        break;
                    }

                    prev_c = reader->text[0];
                    str_advance(reader);
                }
            }
        } break;

        default:{

        } break;
    }

    return result;
}

static Token peek_token(Tokenizer *s){
    Tokenizer place = tokenizer_save(s);
    Token result = next_token(s);
    tokenizer_restore(s, place);
    return result;
}

static bool should_add_extra_newline(Token prev_token, Token token){
    bool result = token.line > 1 && (prev_token.type != Token_Type_Line_Comment || token.line - prev_token.line > 1);
    return result;
}

int main(){
    Buffer memory = {g_memory, Array_Len(g_memory)};

    String reader = file_read_into_memory("./src/atlas.h", &memory);

    Token prev_token = {0};

    Tokenizer tokenizer = make_tokenizer(reader);
    while(peek_token(&tokenizer).type != Token_Type_End_Of_File){
        Token token = next_token(&tokenizer);

        switch(token.type){
            case Token_Type_Line_Comment:{
                if(should_add_extra_newline(prev_token, token))
                    printf("\n");

                printf("//%.*s\n", (int)token.text.size, token.text.text);
            } break;

            case Token_Type_Multi_Line_Comment:{
                if(should_add_extra_newline(prev_token, token))
                    printf("\n");

                printf("/*%.*s*/\n", (int)token.text.size, token.text.text);
            } break;
        }

        prev_token = token;
    }

    return 0;
}
