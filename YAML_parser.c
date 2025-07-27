#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 1024

typedef enum { MAP, SEQ, ITEM } ContextType;

typedef struct 
{
  ContextType type;
  int indent;
} Context;

Context stack[64];
int top = -1;

void 
push(ContextType type, int indent) 
{
  stack[++top].type = type;
  stack[top].indent = indent;
}

void 
pop_to_indent(int indent) 
{
  while (top >= 0 && stack[top].indent >= indent) 
  {
    printf(")\n");
    top--;
  }
}

void 
print_indent() 
{
  for (int i = 0; i <= top; ++i) 
    printf("  ");
}

int 
count_indent(const char *s) 
{
  int count = 0;

  while (*s == ' ') 
  { 
    ++count; ++s; 
  }

  return count;
}

void 
escape_and_print(const char *s) 
{
  putchar('"');
  while (*s) 
  {
    if (*s == '"') 
      printf("\\\"");
    else 
      putchar(*s);
    ++s;
  }
  putchar('"');
}

void 
print_key_value(const char *key, const char *val) 
{
  print_indent();
  printf("(yaml:%s ", key);

  int y, m, d;
  if (sscanf(val, "%d-%d-%d", &y, &m, &d) == 3)
    printf("(make-date %d %02d %02d)", y, m, d);
  else
    escape_and_print(val);
 
  printf(")\n");
}

int 
is_blank(const char *s) 
{
  while (*s) 
  {
    if (!isspace(*s)) 
      return 0;
    ++s;
  }
  return 1;
}

void 
rstrip(char *s) 
{
  int len = strlen(s);
  while (len > 0 && isspace(s[len - 1])) 
    s[--len] = 0;
}

void 
parse_yaml(FILE *f) 
{
  char line[MAX_LINE];
  char last_key[128] = {0};
  int last_indent = -1;
  int pending_block = 0;
  char block_type = 0; // '|' or '>'
  int block_indent = -1;

  while (fgets(line, sizeof(line), f)) 
  {
    if (is_blank(line)) 
      continue;

    rstrip(line);
    int indent = count_indent(line);
    char *content = line + indent;
   
    if (pending_block) 
    {
      if (indent > block_indent) 
      { 
	print_indent(); 
	printf("\"%s\"\n", content + (block_type == '|' ? 0 : 0)); 
	continue;
      } 
      else 
      { 
	printf(")\n");
       	pending_block = 0;
      }
    }
   
    pop_to_indent(indent);

    if (content[0] == '-') 
    {
      content++;
      while (*content == ' ') 
 	content++;
      print_indent();
      printf("(yaml:item\n");
      push(ITEM, indent);
     
      if (strchr(content, ':')) 
      {
        char *colon = strchr(content, ':');
       	*colon = 0;
       	const char *key = content;
       	const char *val = colon + 1;
       	while (*val == ' ') 
	  val++;
       	print_key_value(key, val);
       	strncpy(last_key, key, sizeof(last_key));
      }
      continue;
    }
   
    if (strchr(content, ':')) 
    {
      char *colon = strchr(content, ':');
      *colon = 0;
      const char *key = content;
      const char *val = colon + 1;
      while (*val == ' ') 
	val++;
     
      strncpy(last_key, key, sizeof(last_key));
     
      if (*val == '\0') 
      {
 	print_indent();
       	printf("(yaml:%s\n", key);
       	push(MAP, indent);
      } 
      else if (*val == '|' || *val == '>') 
      {
 	print_indent();
       	printf("(yaml:%s\n", key);
       	push(MAP, indent);
       	block_type = *val;
       	pending_block = 1;
       	block_indent = indent;
      } 
      else 
      {
        print_key_value(key, val);
      }
    }
  }
 
  pop_to_indent(-1);
}


int main(int argc, char *argv[]) 
{
  if (argc < 2) 
  {
    fprintf(stderr, "Usage: %s file.yaml\n", argv[0]);
    return 1;
  }
 
  FILE *f = fopen(argv[1], "r");
  if (!f) 
  {
    perror("Unable to open YAML file");
    return 1;
  }
 
  parse_yaml(f);
  fclose(f);
  return 0;
}
