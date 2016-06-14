#ifndef EXPRESSION_H
#define EXPRESSION_H

typedef
struct expression {
  int func;
  int count;
  union values {
    char *name;
    char *data;
    int num;
    struct expression *ep;
  } values[2];
} expression;

#endif
