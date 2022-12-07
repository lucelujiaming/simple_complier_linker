* 在这个部分中，我们建立所有的变量的符号表。
* 为了支持符号表和作用域，我们使用了下面三个全局变量。
```
  extern std::vector<Symbol> global_sym_stack;  //全局符号栈
  extern std::vector<Symbol> local_sym_stack;   //局部符号栈
  extern std::vector<TkWord> tktable;
```

* 之后，我们针对语法分析的部分，做了下面的修改：
   - 修改direct_declarator_postfix函数，在发现一个数组的时候，添加一个SC_ANOM符号，存储数组大小。
   - 修改parameter_type_list函数，在发现一个函数参数的时候，添加一个SC_PARAMS符号，存储参数。
   - 修改parameter_type_list函数，在处理完函数参数的时候，添加一个SC_ANOM符号，存储？？？？。
   - 修改func_body函数，在进入函数体的时候，放一个匿名符号到局部符号表中。在离开函数体的时候，清空局部符号表。
   - 修改compound_statement函数，在完成一个复合语句的时候，清除本地堆栈。（这里暂时有点问题）
   - 修改primary_expression函数，对于形如"char * test = "AA";"的字符串常量。增加两个符号：一个是匿名的字符类型符号。一个是指针数组。
   - 修改struct_member_declaration函数，这个函数的原名是struct_declaration函数，这个名字有点歧义。
   - 这个函数处理的是结构体成员的声明。对于结构体成员的声明。增加一个结构体成员符号。
   - 修改struct_specifier函数，对于结构体的声明。增加一个结构体符号。
   - 修改external_declaration函数，对于发现的函数定义。增加一个全局普通符号。存储这个函数定义。
   - 修改external_declaration函数，对于发现的函数声明。增加一个全局普通符号。存储这个函数声明。

* 下面记录一下我的实验结果。首先是测试程序。
```
     1.    struct point
     2.    {
     3.       short x;
     4.       short y;
     5.    };
     6.    
     7.    void main(){
     8.       struct point pt;
     9.       char *pstr;
    10.       return 1;
    11.    } 
```
* 首先在初始化的时候，我们添加了一个默认的指针类型符号。符号表内容如下：
```
     local_sym_stack[0].v = 10000000 c = -1 type = 1
```
* 可以看到存储类型为匿名符号0x10000000。类型编码为1 - T_CHAR
* 之后我们解析了struct point结构体。 
```
     1.    struct point
     2.    { 
```
* 符号表内容如下： 
```
       global_sym_stack[0].v = 2000002C c = 2 type = 6 
```
* 可以看到我们多了一个全局符号。
* 可以看到一个结构体。存储类型为结构体符号0x2000002C。类型编码为6 - T_STRUCT
* 之后我们解析了struct point结构体的成员。 
```
    3.       short x;
    4.       short y;
    5.    };
    6.    
```
* 符号表内容如下：
```
     global_sym_stack[0].v = 2000002C c = 2 type = 6
     global_sym_stack[1].v = 4000002C c = 0 type = 2
     global_sym_stack[2].v = 4000002C c = 2 type = 2
      local_sym_stack[0].v = 10000000 c = -1 type = 1
```
 * 可以看到我们多了三个全局符号。
 * 可以看到一个结构体。存储类型为结构体符号0x2000002C。类型编码为6 - T_STRUCT
 * 可以看到两个结构体成员。存储类型为结构体成员0x4000002C。类型编码为2 - 短整型short
 * 之后我们进入了main函数。
```
      7.    void main(){
```
 * 符号表内容如下：
```
    global_sym_stack[0].v = 2000002C c = 2 type = 6
    global_sym_stack[1].v = 4000002C c = 0 type = 2
    global_sym_stack[2].v = 4000002C c = 2 type = 2
    global_sym_stack[3].v = 10000000 c = 0 type = 0
    global_sym_stack[4].v = 0000002C c = 0 type = 5
     local_sym_stack[0].v = 10000000 c = -1 type = 1
```
 * 可以看到我们多了两个全局符号。
 * 可以看到一个全局符号的存储类型为匿名符号0x10000000。类型编码为0 - T_INT整型。
 * 该添加过程位于parameter_type_list中，也就是我们只要分析到一个函数，就为这个函数添加一个匿名符号。
 * 可以看到是一个函数。存储类型为0x0000002C。类型编码为5 - T_FUNC
 * 继续执行，符号表内容如下：
```
     global_sym_stack[0].v = 2000002C c = 2 type = 6
     global_sym_stack[1].v = 4000002C c = 0 type = 2
     global_sym_stack[2].v = 4000002C c = 2 type = 2
     global_sym_stack[3].v = 10000000 c = 0 type = 0
     global_sym_stack[4].v = 0000002C c = 0 type = 5
      local_sym_stack[0].v = 10000000 c = -1 type = 1
      local_sym_stack[1].v = 10000000 c = 0 type = 0
```
 * 可以看到我们多了一个本地符号。
 * 可以看到一个本地符号的存储类型为匿名符号0x10000000。类型编码为0 - T_INT整型。
 * 这个本地符号，可以理解为一个占位符。在compound_statement中被擦除。
 * 之后我们声明了一个结构体。
```
       8.       struct point pt;
```
 * 符号表内容如下：
```
    global_sym_stack[0].v = 2000002C c = 2 type = 6
    global_sym_stack[1].v = 4000002C c = 0 type = 2
    global_sym_stack[2].v = 4000002C c = 2 type = 2
    global_sym_stack[3].v = 10000000 c = 0 type = 0
    global_sym_stack[4].v = 0000002C c = 0 type = 5
    global_sym_stack[5].v = 2000002C c = -1 type = 6
     local_sym_stack[0].v = 10000000 c = -1 type = 1
     local_sym_stack[1].v = 10000000 c = 0 type = 0
```
 * 可以看到我们多了一个全局结构体符号。
 * 可以看到struct point pt的存储类型为结构体符号0x2000002C。类型编码为6 - T_STRUCT
 * 之后我们声明了一个字符串指针：
```
      9.       char *pstr;
```
 * 符号表内容如下：
```
   global_sym_stack[0].v = 2000002C c = 2 type = 6
   global_sym_stack[1].v = 4000002C c = 0 type = 2
   global_sym_stack[2].v = 4000002C c = 2 type = 2
   global_sym_stack[3].v = 10000000 c = 0 type = 0
   global_sym_stack[4].v = 0000002C c = 0 type = 5
   global_sym_stack[5].v = 2000002C c = -1 type = 6
   global_sym_stack[6].v = 10000000 c = -1 type = 0
    local_sym_stack[0].v = 10000000 c = -1 type = 1
    local_sym_stack[1].v = 10000000 c = 0 type = 0
```
 * 可以看到我们多了一个全局符号。
 * 可以看到char *pstr的存储类型为匿名符号0x10000000。类型编码为0 - T_INT整型。这是一个指针。
 * 这个符号通过mk_pointer添加。也就是我们每发现一个指针类型就会增添一个符号。
 * 之后我们执行函数返回：
```
      10.       return 1;
      11.    } 
```
 * 符号表内容如下：
```
   global_sym_stack[0].v = 2000002C c = 2 type = 6
   global_sym_stack[1].v = 4000002C c = 0 type = 2
   global_sym_stack[2].v = 4000002C c = 2 type = 2
   global_sym_stack[3].v = 10000000 c = 0 type = 0
   global_sym_stack[4].v = 0000002C c = 0 type = 5
   global_sym_stack[5].v = 2000002C c = -1 type = 6
  global_sym_stack[6].v = 10000000 c = -1 type = 0
```
 * 可以看到我们清除了本地堆栈。
 * 程序执行完成。

* 至于，单词TkWord包含很多种可能。既有可能是运算符及分隔符，常量，也有可能是普通变量和结构体。
* 比方说，对于下面这个程序：
```
struct point
{
	short x;
	short y;
};

void main(){
    int a[5];
	struct point pt;
	char *pstr;
	char * test = "AA";
	return 1;
}
```
* 他的单词表结果如下：
```
tktable[0].spelling = +
......
tktable[42].spelling = __stdcall
tktable[43].spelling = point
tktable[44].spelling = x
tktable[45].spelling = y
tktable[46].spelling = main
tktable[47].spelling = a
tktable[48].spelling = pt
tktable[49].spelling = pstr
tktable[50].spelling = test
```

