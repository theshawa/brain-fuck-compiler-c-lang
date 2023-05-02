#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *COMMANDS = "+-<>[].,";
const size_t CELL_COUNT = 300000;
const int DEV_MODE = 0;

typedef struct Error
{
    enum ErrorType
    {
        ERROR_NONE = 0,
        ERROR_FILE,
        ERROR_LOOP
    } type;
    char *message;
} Error;

#define ErrorPrep(err, t, m) \
    (err).type = (t);        \
    (err).message = (m);

Error ok = {
    ERROR_NONE,
    NULL,
};

typedef struct Token
{
    char value;
    struct Token *next;
} Token;

Token *generate_token_space()
{
    Token *t = malloc(sizeof(Token));
    return t;
}

typedef uint8_t Cell;

typedef struct Loop
{
    Token *starting_token;
    struct Loop *parent;
} Loop;

void print_usage(char *ex_path)
{
    printf("No file selected.\nUSAGE: %s <path_to_file>", ex_path);
}

size_t file_size(FILE *file)
{
    fpos_t original;

    if (fgetpos(file, &original) != 0)
    {
        printf("fgetpos() faild\nerror-code: %i\n", errno);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);

    if (fsetpos(file, &original) != 0)
    {
        printf("fsetpos() faild\nerror-code: %i\n", errno);
        exit(1);
    }

    return size;
}

char *file_content(FILE *file)
{
    size_t size = file_size(file);

    char *content = malloc(size + 1);
    char *writer = content;

    size_t bytes_read = 0;

    while (bytes_read < size)
    {
        size_t current_bytes_read = fread(writer, 1, size - bytes_read, file);
        if (ferror(file))
        {
            printf("Error while reading\nerror-code: %i\n", errno);
            free(content);
            exit(1);
        }

        bytes_read += current_bytes_read;
        writer += current_bytes_read;

        if (feof(file))
        {
            break;
        }
    }

    *writer = '\0';

    return content;
}

Token *lex(char *content)
{
    Token *tokens = NULL;
    Token *tokens_it = tokens;
    char *current_char = content;

    while (1)
    {
        size_t whitespace = strcspn(current_char, COMMANDS);
        current_char += whitespace;

        Token token = {
            *current_char,
            NULL,
        };

        if (!tokens)
        {
            tokens = generate_token_space();
            memcpy(tokens, &token, sizeof(Token));
            tokens_it = tokens;
        }
        else
        {
            tokens_it->next = generate_token_space();
            memcpy(tokens_it->next, &token, sizeof(Token));
            tokens_it = tokens_it->next;
        }

        current_char += 1;
        if (*current_char == '\0')
        {
            break;
        }
    }

    return tokens;
}

void print_tokens(Token *tokens)
{
    Token *current_token = tokens;
    while (current_token)
    {
        printf("Token: %c\n", current_token->value);
        current_token = current_token->next;
    }
}

void compile(Token *current_token, Cell *pointer, Cell *start, Cell *end)
{

    Loop *loop = NULL;
    Loop *loop_it = loop;

    while (current_token)
    {
        char value = current_token->value;
        if (value == '>')
        {
            if (pointer < end)
            {
                pointer += sizeof(Cell);
            }
        }
        else if (value == '<')
        {
            if (pointer > start)
            {
                pointer -= sizeof(Cell);
            }
        }
        else if (value == '+')
        {
            (*pointer)++;
        }
        else if (value == '-')
        {
            (*pointer)--;
        }
        else if (value == '.')
        {
            if (DEV_MODE)
            {
                printf("%i %c\n", *pointer, *pointer);
            }
            else
            {
                putchar(*pointer);
            }
        }
        else if (value == ',')
        {
            *pointer = getchar();
        }
        else if (value == '[')
        {
            if (!loop_it)
            {
                loop = malloc(sizeof(Loop));
                loop->parent = NULL;
                loop->starting_token = current_token;
                loop_it = loop;
            }
            else
            {
                Loop new_loop = {
                    current_token,
                    loop_it,
                };
                loop_it = &new_loop;
                current_token = loop_it->starting_token;
            }
        }
        else if (value == ']')
        {
            if (*pointer == 0)
            {
                Loop *parent = loop_it->parent;
                free(loop_it);
                loop_it = parent;
            }
            else
            {
                current_token = loop_it->starting_token;
            }
        }

        current_token = current_token->next;
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        print_usage(argv[0]);
        return 0;
    }

    printf("Compiling...\n\n");

    FILE *file;

    fopen_s(&file, argv[1], "r");

    if (!file)
    {
        printf("Unable to open the file at '%s'\nerror-code: %i", argv[1], errno);
        return 0;
    }

    char *content = file_content(file);
    if (DEV_MODE)
    {
        printf("Content:\n%s\n", content);
    }

    Token *tokens = lex(content);
    if (DEV_MODE)
    {
        print_tokens(tokens);
    }

    size_t mem_size = sizeof(Cell) * CELL_COUNT;
    Cell *cells = malloc(mem_size);
    memset(cells, 0, mem_size);

    Cell *pointer = cells;

    compile(tokens, pointer, cells, cells + mem_size);

    fclose(file);
    free(content);
    free(cells);

    printf("\n\nOperation Successful.\nCompiler developed by Theshawa Dasun(https://theshawa-dev.web.app)");

    return 0;
}
