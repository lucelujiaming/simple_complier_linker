// COFF文件格式
// 
// 首先COFF文件由五个部分组成。分别是：
// 1. COFF文件头。为一个IMAGE_FILE_HEADER结构体类型。
// 2. 节头表。为多个IMAGE_SECTION_HEADER结构体类型。数量定义在COFF文件头中。
//    因此上，下面提到的"节"就是英语Section的意思。
// 3. 节数据。和节头一一对应。
// 4. 符号表。为多个IMAGE_SECTION_HEADER结构体类型。
// 5. 字符串表。

// 1. COFF文件头结构体IMAGE_FILE_HEADER定义如下：
typedef struct _IMAGE_FILE_HEADER {
	// 标识目标机器类型的数字。要获取更多信息，请参考7.1.3.1节“机器类型”。
	WORD    Machine;
	// 节的数目。它给出了节表的大小，而节表紧跟着文件头。注意Windows加载器限制节的最大数目为96
	WORD    NumberOfSections;
	// Unix时间戳，它指出文件何时被创建。
	DWORD   TimeDateStamp;
	// 符号表的文件偏移。如果不存在COFF符号表，此值为0。
	// 对于映像文件来说，此值应该为0，因为已经不赞成使用COFF调试信息了。
	DWORD   PointerToSymbolTable;
	// 符号表中的元素数目。由于字符串表紧跟符号表，所以可以利用这个值来定位字符串表。
	// 对于映像文件来说，此值应该为0，因为已经不赞成使用COFF调试信息了。
	DWORD   NumberOfSymbols;
	// 可选文件头的大小。可执行文件需要可选文件头而目标文件并不需要。
	// 对于目标文件来说，此值应该为0。
	// 要获取可选文件头格式的详细描述，请参考10.2.6节“可选文件头(仅适用于映像文件)”
	WORD    SizeOfOptionalHeader;
	// 指示文件属性的标志。对于特定的标志，请参考7.1.3.2节“文件属性标志”
	WORD    Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

// 1.1 机器类型：定义CPU类型的Machine定义如下：
#define IMAGE_FILE_MACHINE_UNKNOWN           0
#define IMAGE_FILE_MACHINE_I386              0x014c  // Intel 386.
#define IMAGE_FILE_MACHINE_R3000             0x0162  // MIPS little-endian, 0x160 big-endian
#define IMAGE_FILE_MACHINE_R4000             0x0166  // MIPS little-endian
#define IMAGE_FILE_MACHINE_R10000            0x0168  // MIPS little-endian
#define IMAGE_FILE_MACHINE_WCEMIPSV2         0x0169  // MIPS little-endian WCE v2
#define IMAGE_FILE_MACHINE_ALPHA             0x0184  // Alpha_AXP
#define IMAGE_FILE_MACHINE_POWERPC           0x01F0  // IBM PowerPC Little-Endian
#define IMAGE_FILE_MACHINE_SH3               0x01a2  // SH3 little-endian
#define IMAGE_FILE_MACHINE_SH3E              0x01a4  // SH3E little-endian
#define IMAGE_FILE_MACHINE_SH4               0x01a6  // SH4 little-endian
#define IMAGE_FILE_MACHINE_ARM               0x01c0  // ARM Little-Endian
#define IMAGE_FILE_MACHINE_THUMB             0x01c2
#define IMAGE_FILE_MACHINE_IA64              0x0200  // Intel 64
#define IMAGE_FILE_MACHINE_MIPS16            0x0266  // MIPS
#define IMAGE_FILE_MACHINE_MIPSFPU           0x0366  // MIPS
#define IMAGE_FILE_MACHINE_MIPSFPU16         0x0466  // MIPS
#define IMAGE_FILE_MACHINE_ALPHA64           0x0284  // ALPHA64
#define IMAGE_FILE_MACHINE_AXP64             IMAGE_FILE_MACHINE_ALPHA64
// 在Windows平台上，我们选择Intel 386 - IMAGE_FILE_MACHINE_I386

// 1.2 文件属性标志：Characteristics域包含指示目标文件或映像文件属性的标志。

		// 仅适用于映像文件，适用于WindowsCE、Microsoft WindowsNT_及其后继操作系统。
		// 它表明此文件不包含基址重定位信息，因此必须被加载到其首选基地址上。
		// 如果基地址不可用，加载器会报错。链接器默认会移除可执行(EXE)文件中的重定位信息。
#define IMAGE_FILE_RELOCS_STRIPPED           0x0001  // Relocation info stripped from file.
		// 仅适用于映像文件，它表明此映像文件是合法的，可以被运行。如果未设置此标志，表明出现了链接器错误。
#define IMAGE_FILE_EXECUTABLE_IMAGE          0x0002  // File is executable  (i.e. no unresolved externel references).
		// COFF行号信息已经被移除，不赞成使用此标志，它应该为0
#define IMAGE_FILE_LINE_NUMS_STRIPPED        0x0004  // Line nunbers stripped from file.
		// COFF符号表中有关局部符号的项已经被移除，不赞成使用此标志，它应该为0
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED       0x0008  // Local symbols stripped from file.
		// 此标志已经被舍弃，它用于调整工作集。Windows 2000 及其后继操作系统不赞成使用此标志，它应该为0
#define IMAGE_FILE_AGGRESIVE_WS_TRIM         0x0010  // Agressively trim working set
		// 应用程序可以处理大于2GB的地址。
#define IMAGE_FILE_LARGE_ADDRESS_AWARE       0x0020  // App can handle >2gb addresses
		// 0x0040标志保留供将来使用。
		// 小端：在内存中，最不重要位(LSB)在最重要位(MSB)前面。不赞成使用此标志，它应该为0。
#define IMAGE_FILE_BYTES_REVERSED_LO         0x0080  // Bytes of machine word are reversed.
		// 机器类型基于32位字体系结构。
#define IMAGE_FILE_32BIT_MACHINE             0x0100  // 32 bit word machine.
		// 调试信息已经从此映像文件中移除。
#define IMAGE_FILE_DEBUG_STRIPPED            0x0200  // Debugging info stripped from file in .DBG file
		// 如果此映像文件在可移动介质上，完全加载它并把它复制到交换文件中。
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP   0x0400  // If Image is on removable media, copy and run from the swap file.
		// 如果此映像文件在网络介质上，完全加载它并把它复制到交换文件中。
#define IMAGE_FILE_NET_RUN_FROM_SWAP         0x0800  // If Image is on Net, copy and run from the swap file.
		// 此映像文件是系统文件，而不是用户程序。
#define IMAGE_FILE_SYSTEM                    0x1000  // System File.
		// 此映像文件是动态链接库(DLL) 。这样的文件总被认为是可执行文件，尽管它们并不能直接被运行。
#define IMAGE_FILE_DLL                       0x2000  // File is a DLL.
		// 此文件只能运行于单处理器机器上。
#define IMAGE_FILE_UP_SYSTEM_ONLY            0x4000  // File should only be run on a UP machine
		// 大端：在内存中，MSB在LSB前面。不赞成使用此标志。
#define IMAGE_FILE_BYTES_REVERSED_HI         0x8000  // Bytes of machine word are reversed.


// 2. 节头表：
// 节头表的每一行等效于一个节头。这个表紧跟可选文件头(如果存在)。
// 节头表中的元素数目由文件头中的Number Of Sections域给出，而元素的编号是从1开始的。
// 表示代码节和数据节的元素的顺序由链接器决定。
// 在映像文件中，每个节的VA值必须由链接器决定。这样能够保证这些节位置相邻且按升序排列，
// 并且这些VA值必须是可选文件头中Section Alignment域的倍数。
// 每个节头(节表项) 格式如下，共40个字节，IMAGE_SECTION_HEADER结构体定义如下：
#define IMAGE_SIZEOF_SHORT_NAME              8

typedef struct _IMAGE_SECTION_HEADER {
	// 这是一个8字节的UTF-8 编码的字符串，不足8字节时用NULL填充。
	// 如果它正好是8字节，那就没有最后的NULL字符。
	// 如果名称更长，这个域中是一个斜杠(/) 后跟一个用ASCII码表示的十进制数，
	// 这个十进制数表示字符串表中的偏移。
	// 可执行映像不使用字符串表也不支持长度超过8字节的节名。
	// 如果目标文件中有长节名的节最后要出现在可执行文件中，那么相应的长节名会被截断。
    BYTE    Name[IMAGE_SIZEOF_SHORT_NAME];
	// 当加载进内存时这个节的总大小。如果此值比SizeOf RawData大，那么多出的部分用0填充。
	// 这个域仅对可执行映像是合法的，对于目标文件来说，它应该为0。
    union {
            DWORD   PhysicalAddress;
            DWORD   VirtualSize;
    } Misc;
	// 对于可执行映像来说，这个域的值是这个节被加载进内存之后它的第一个字节相对于映像基址的偏移地址。
	// 对于目标文件来说，这个域的值是没有重定位之前其第一个字节的地址；
	// 为了简单起见，编译器应该把此值设置为0；
	// 否则这个值是个任意值，但是在重定位时应该从偏移地址中减去这个值。
    DWORD   VirtualAddress;
	// (对于目标文件来说)节的大小或者(对于映像文件来说)磁盘文件中已初始化数据的大小。
	// 对于可执行映像来说，它必须是可选文件头中FileAlignment域的倍数。
	// 如果它小于VirtualSize域的值，余下的部分用0填充。
	// 由于SizeOfRawData域要向上舍入，但是VirtualSize域并不舍入，
	// 因此可能出现SizeOf RawData域大于Virtual Size域的情况。
	// 当节中仅包含未初始化的数据时，这个域应该为0。
    DWORD   SizeOfRawData;
	// 指向COFF文件中节的第一个页面的文件指针。
	// 对于可执行映像来说，它必须是可选文件头中File Alignment域的倍数。
	// 对于目标文件来说，要获得最好的性能，此值应该按4字节边界对齐。
	// 当节中仅包含未初始化的数据时，这个域应该为0。
    DWORD   PointerToRawData;
	// 指向节中重定位项开头的文件指针。对于可执行文件或者没有重定位项的文件来说，此值应该为0。
    DWORD   PointerToRelocations;
	// 指向节中行号项开头的文件指针。如果没有COFF行号信息，此值应该为0。
	// 对于映像来说，此值应该为0，因为已经不赞成使用COFF调试信息了。
    DWORD   PointerToLinenumbers;
	// 节中重定位项的个数。对于可执行映像来说，此值应该为0。
    WORD    NumberOfRelocations;
	// 节中行号项的个数。对于映像来说，此值应该为0，因为已经不赞成使用COFF调试信息了。
    WORD    NumberOfLinenumbers;
	// 描述节特征的标志。要获取更多信息，请参考7.1.4.1节“节标志”。
    DWORD   Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

// 2.1 描述节特征的标志定义如下：<略>
//
// Section characteristics.
//
//      IMAGE_SCN_TYPE_REG                   0x00000000  // Reserved.
//      IMAGE_SCN_TYPE_DSECT                 0x00000001  // Reserved.
//      IMAGE_SCN_TYPE_NOLOAD                0x00000002  // Reserved.
//      IMAGE_SCN_TYPE_GROUP                 0x00000004  // Reserved.
#define IMAGE_SCN_TYPE_NO_PAD                0x00000008  // Reserved.
//      IMAGE_SCN_TYPE_COPY                  0x00000010  // Reserved.

#define IMAGE_SCN_CNT_CODE                   0x00000020  // Section contains code.
#define IMAGE_SCN_CNT_INITIALIZED_DATA       0x00000040  // Section contains initialized data.
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA     0x00000080  // Section contains uninitialized data.

#define IMAGE_SCN_LNK_OTHER                  0x00000100  // Reserved.
#define IMAGE_SCN_LNK_INFO                   0x00000200  // Section contains comments or some other type of information.
//      IMAGE_SCN_TYPE_OVER                  0x00000400  // Reserved.
#define IMAGE_SCN_LNK_REMOVE                 0x00000800  // Section contents will not become part of image.
#define IMAGE_SCN_LNK_COMDAT                 0x00001000  // Section contents comdat.
//                                           0x00002000  // Reserved.
//      IMAGE_SCN_MEM_PROTECTED - Obsolete   0x00004000
#define IMAGE_SCN_NO_DEFER_SPEC_EXC          0x00004000  // Reset speculative exceptions handling bits in the TLB entries for this section.
#define IMAGE_SCN_GPREL                      0x00008000  // Section content can be accessed relative to GP
#define IMAGE_SCN_MEM_FARDATA                0x00008000
//      IMAGE_SCN_MEM_SYSHEAP  - Obsolete    0x00010000
#define IMAGE_SCN_MEM_PURGEABLE              0x00020000
#define IMAGE_SCN_MEM_16BIT                  0x00020000
#define IMAGE_SCN_MEM_LOCKED                 0x00040000
#define IMAGE_SCN_MEM_PRELOAD                0x00080000

#define IMAGE_SCN_ALIGN_1BYTES               0x00100000  //
#define IMAGE_SCN_ALIGN_2BYTES               0x00200000  //
#define IMAGE_SCN_ALIGN_4BYTES               0x00300000  //
#define IMAGE_SCN_ALIGN_8BYTES               0x00400000  //
#define IMAGE_SCN_ALIGN_16BYTES              0x00500000  // Default alignment if no others are specified.
#define IMAGE_SCN_ALIGN_32BYTES              0x00600000  //
#define IMAGE_SCN_ALIGN_64BYTES              0x00700000  //
#define IMAGE_SCN_ALIGN_128BYTES             0x00800000  //
#define IMAGE_SCN_ALIGN_256BYTES             0x00900000  //
#define IMAGE_SCN_ALIGN_512BYTES             0x00A00000  //
#define IMAGE_SCN_ALIGN_1024BYTES            0x00B00000  //
#define IMAGE_SCN_ALIGN_2048BYTES            0x00C00000  //
#define IMAGE_SCN_ALIGN_4096BYTES            0x00D00000  //
#define IMAGE_SCN_ALIGN_8192BYTES            0x00E00000  //
// Unused                                    0x00F00000

#define IMAGE_SCN_LNK_NRELOC_OVFL            0x01000000  // Section contains extended relocations.
#define IMAGE_SCN_MEM_DISCARDABLE            0x02000000  // Section can be discarded.
#define IMAGE_SCN_MEM_NOT_CACHED             0x04000000  // Section is not cachable.
#define IMAGE_SCN_MEM_NOT_PAGED              0x08000000  // Section is not pageable.
#define IMAGE_SCN_MEM_SHARED                 0x10000000  // Section is shareable.
#define IMAGE_SCN_MEM_EXECUTE                0x20000000  // Section is executable.
#define IMAGE_SCN_MEM_READ                   0x40000000  // Section is readable.
#define IMAGE_SCN_MEM_WRITE                  0x80000000  // Section is writeable.

// 3. 节数据
// 典型的COFF节就是普通的代码或者数据。但是也有一些特殊的节。包括：<略>
// 首先是代码节。即.text节。
// .data节、.r data节和.bss节都属于数据节，从7.1.4.2节可知，
//     .data节存储可读可写需要初始化的数据，.rdata节存储只读数据，
//     .bss节存储可读可写不需要进行始化的数据，
//     .idata节是导入节，存储导入数据。

// 4. 符号表。为多个IMAGE_SECTION_HEADER结构体类型。定义如下：
typedef struct _IMAGE_SECTION_HEADER {
    BYTE    Name[IMAGE_SIZEOF_SHORT_NAME];
    union {
            DWORD   PhysicalAddress;
            DWORD   VirtualSize;
    } Misc;
    DWORD   VirtualAddress;
    DWORD   SizeOfRawData;
    DWORD   PointerToRawData;
    DWORD   PointerToRelocations;
    DWORD   PointerToLinenumbers;
    WORD    NumberOfRelocations;
    WORD    NumberOfLinenumbers;
    DWORD   Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;


// 5. 字符串表。























