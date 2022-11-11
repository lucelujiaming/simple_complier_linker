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
*  第一个例子是2_myFirstProgram。做了一个基本的表达式检验。包括检测括号是否匹配。
*  第二个例子应该是4_token_process。也就是词法分析。
   - 这个例子的核心就是get_token。调用它就会读取一个单词。并给出单词的类型。
   - 之后，调用color_token，给这个单词上色。
* 第三个例子应该是5_translation_unit。也就是语法分析。
   - 只要熟悉C语言的语句就不难理解这部分的逻辑。
* 第四个例子是6_symbol_table。支持符号表和作用域。
   - 这个部分的代码比较杂乱。作者也是没有给出工程代码。我自己添加了一个工程。
   - 还有就是，这里有一个错误。就是如果这份代码直接加入到工程中是无法执行。会报错。
     - 解决方法是在程序初始化的时候调用init_lex函数，在单词表中加入系统保留的单词。
     - 而随书代码中给出的这个init_lex函数有一个问题。就是缺少对于WHILE关键字的处理。
     - 虽然从SC的语法规范中来说。SC的语法是不支持WHILE关键字的。但是在e_TokenCode的枚举定义中，这个关键字确是存在。
     - 而我们在代码中，访问单词表tktable是使用e_TokenCode的枚举作为下标进行访问。
     - 当我们使用下标44访问一个只有44个元素，下标范围是从0到43的vector对象。会触发访问越界。会导致程序崩溃。
* 第五个例子是7_coff_generator。这一章和前面的章节没啥关系。主要讲了coff文件的结构。也就是Windows使用的目标文件格式。
     - 我们只需要使用下面的代码就可以完成测试程序。生成一个obj文件。当然这个文件的每一个节的内容是空的。
```
      int main(int argc, char* argv[])
      {
         init_coff();
         write_obj("write_obj.obj"); 
         free_sections();
         return 0;
      }
```
* 第六个例子是8_x86_generator。这一章和前面的章节没啥关系。主要讲了x86使用的机器语言。注意这里说的不是汇编语言。也就是机器码。
* 第七个例子是9_semantic_analyser。也就是语义分析。

