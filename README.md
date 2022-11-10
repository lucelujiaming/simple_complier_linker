# simple_complier_linker

# 这个代码库主要是我自己重写的《自己动手写编译器链接器》。
# 我认为原来的代码有几点实现的不好。
1.  第一个是分词器。这个可以写的更加优雅一点。
2.  第二个就是变量栈，这个明显是可以用一个vector<string>处理掉的。
3.  同理还有符号表，这个明显也是可以用一个vector<Symbol>处理掉的。
4.  还有就是这个的第六七八三章，是没有工程的。因此上，很难观察到一些中间效果。

# 下面是代码目录。按照章节分开。
1. 2_myFirstProgram
2. 4_token_process
3. 5_translation_unit
4. 6_symbol_table
5. 7_coff_generator
6. 8_x86_generator
7. 9_semantic_analyser

# 下面解释一下：
*  第一个例子是myFIrstProgram。做了一个基本的表达式检验。包括检测括号是否匹配。
*  第二个例子应该是token_process。也就是词法分析。
   - 这个例子的核心就是get_token。调用它就会读取一个单词。并给出单词的类型。
   - 之后，调用color_token，给这个单词上色。
* 第三个例子应该是translation_unit。也就是语法分析。
   - 只要熟悉C语言的语句就不难理解这部分的逻辑。
* 第四个例子是symbol_table。支持符号表和作用域。
   - 这个部分的代码比较杂乱。作者也是没有给出工程代码。我自己添加了一个工程。
