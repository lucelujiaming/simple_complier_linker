# simple_complier_linker

# 这个代码库主要是我自己重写的《自己动手写编译器链接器》。
# 我认为原来的代码有几点实现的不好。
##  第一个是分词器。这个可以写的更加优雅一点。
##  第二个就是变量栈，这个明显是可以用一个vector<string>处理掉的。
##  同理还有符号表，这个明显也是可以用一个vector<Symbol>处理掉的。
##  还有就是这个的第六七八三章，是没有工程的。因此上，很难观察到一些中间效果。

# 下面是代码目录。按照章节分开。
## 1. 2_myFirstProgram
## 2. 4_token_process
## 3. 5_translation_unit
## 4. 6_symbol_table
## 5. 7_coff_generator
## 6. 8_x86_generator
## 7. 9_semantic_analyser

# 下面解释一下：
##  第一个例子是myFIrstProgram。做了一个基本的表达式检验。包括检测括号是否匹配。
##  第二个例子应该是token_process。也就是词法分析。
##    这个例子的核心就是get_token。调用它就会读取一个单词。并给出单词的类型。
##    之后，调用color_token，给这个单词上色。
##  第三个例子应该是translation_unit。也就是语法分析。
##    只要熟悉C语言的语句就不难理解这部分的逻辑。
## 第四个例子是symbol_table。在这个部分中，我们建立所有的变量的符号表。
##    下面记录一下我的实验结果。首先是测试程序。
##         1.    struct point
##         2.    {
##         3.       short x;
##         4.       short y;
##         5.    };
##         6.    
##         7.    void main(){
##         8.       struct point pt;
##         9.       char *pstr;
##        10.       return 1;
##        11.    } 
##    首先在初始化的时候，我们添加了一个默认的指针类型符号。符号表内容如下：
##      local_sym_stack[0].v = 10000000 c = -1 type = 1
##    可以看到存储类型为匿名符号0x10000000。类型编码为1 - T_CHAR
##    之后我们解析了struct point结构体。
##         1.    struct point
##         2.    {
##    符号表内容如下：
##         global_sym_stack[0].v = 2000002C c = 2 type = 6
##    可以看到我们多了一个全局符号。
##    可以看到一个结构体。存储类型为结构体符号0x2000002C。类型编码为6 - T_STRUCT
##    之后我们解析了struct point结构体的成员。
##         3.       short x;
##         4.       short y;
##         5.    };
##         6.    
##    符号表内容如下：
##         global_sym_stack[0].v = 2000002C c = 2 type = 6
##         global_sym_stack[1].v = 4000002C c = 0 type = 2
##         global_sym_stack[2].v = 4000002C c = 2 type = 2
##          local_sym_stack[0].v = 10000000 c = -1 type = 1
##    可以看到我们多了三个全局符号。
##    可以看到一个结构体。存储类型为结构体符号0x2000002C。类型编码为6 - T_STRUCT
##    可以看到两个结构体成员。存储类型为结构体成员0x4000002C。类型编码为2 - 短整型short
##    之后我们进入了main函数。
##         7.    void main(){
##    符号表内容如下：
##         global_sym_stack[0].v = 2000002C c = 2 type = 6
##         global_sym_stack[1].v = 4000002C c = 0 type = 2
##         global_sym_stack[2].v = 4000002C c = 2 type = 2
##         global_sym_stack[3].v = 10000000 c = 0 type = 0
##         global_sym_stack[4].v = 0000002C c = 0 type = 5
##          local_sym_stack[0].v = 10000000 c = -1 type = 1
##    可以看到我们多了两个全局符号。
##    可以看到一个全局符号的存储类型为匿名符号0x10000000。类型编码为0 - T_INT整型。
##    该添加过程位于parameter_type_list中，也就是我们只要分析到一个函数，就为这个函数添加一个匿名符号。
##    可以看到是一个函数。存储类型为0x0000002C。类型编码为5 - T_FUNC
##    继续执行，符号表内容如下：
##         global_sym_stack[0].v = 2000002C c = 2 type = 6
##         global_sym_stack[1].v = 4000002C c = 0 type = 2
##         global_sym_stack[2].v = 4000002C c = 2 type = 2
##         global_sym_stack[3].v = 10000000 c = 0 type = 0
##         global_sym_stack[4].v = 0000002C c = 0 type = 5
##          local_sym_stack[0].v = 10000000 c = -1 type = 1
##          local_sym_stack[1].v = 10000000 c = 0 type = 0
##    可以看到我们多了一个本地符号。
##    可以看到一个本地符号的存储类型为匿名符号0x10000000。类型编码为0 - T_INT整型。
##    这个本地符号，可以理解为一个占位符。在compound_statement中被擦除。
##    之后我们声明了一个结构体。
##         8.       struct point pt;
##    符号表内容如下：
##         global_sym_stack[0].v = 2000002C c = 2 type = 6
##         global_sym_stack[1].v = 4000002C c = 0 type = 2
##         global_sym_stack[2].v = 4000002C c = 2 type = 2
##         global_sym_stack[3].v = 10000000 c = 0 type = 0
##         global_sym_stack[4].v = 0000002C c = 0 type = 5
##         global_sym_stack[5].v = 2000002C c = -1 type = 6
##          local_sym_stack[0].v = 10000000 c = -1 type = 1
##          local_sym_stack[1].v = 10000000 c = 0 type = 0
##    可以看到我们多了一个全局结构体符号。
##    可以看到struct point pt的存储类型为结构体符号0x2000002C。类型编码为6 - T_STRUCT
##    之后我们声明了一个字符串指针：
##         9.       char *pstr;
##    符号表内容如下：
##         global_sym_stack[0].v = 2000002C c = 2 type = 6
##         global_sym_stack[1].v = 4000002C c = 0 type = 2
##         global_sym_stack[2].v = 4000002C c = 2 type = 2
##         global_sym_stack[3].v = 10000000 c = 0 type = 0
##         global_sym_stack[4].v = 0000002C c = 0 type = 5
##         global_sym_stack[5].v = 2000002C c = -1 type = 6
##         global_sym_stack[6].v = 10000000 c = -1 type = 0
##          local_sym_stack[0].v = 10000000 c = -1 type = 1
##          local_sym_stack[1].v = 10000000 c = 0 type = 0
##    可以看到我们多了一个全局符号。
##    可以看到char *pstr的存储类型为匿名符号0x10000000。类型编码为0 - T_INT整型。这是一个指针。
##    这个符号通过mk_pointer添加。也就是我们每发现一个指针类型就会增添一个符号。
##    之后我们执行函数返回：
##        10.       return 1;
##        11.    } 
##    符号表内容如下：
##         global_sym_stack[0].v = 2000002C c = 2 type = 6
##         global_sym_stack[1].v = 4000002C c = 0 type = 2
##         global_sym_stack[2].v = 4000002C c = 2 type = 2
##         global_sym_stack[3].v = 10000000 c = 0 type = 0
##         global_sym_stack[4].v = 0000002C c = 0 type = 5
##         global_sym_stack[5].v = 2000002C c = -1 type = 6
##         global_sym_stack[6].v = 10000000 c = -1 type = 0
##    可以看到我们清除了本地堆栈。
##    程序执行完成。

