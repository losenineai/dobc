
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.4.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 189 of yacc.c  */
#line 16 "slghparse.y"

#include "slgh_compile.hh"

#define YYERROR_VERBOSE

  extern SleighCompile *slgh;
  extern int4 actionon;
  extern FILE *yyin;
  extern int yydebug;
  extern int yylex(void);
  extern int yyerror(const char *str );


/* Line 189 of yacc.c  */
#line 87 "slghparse.cc"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     OP_BOOL_OR = 258,
     OP_BOOL_XOR = 259,
     OP_BOOL_AND = 260,
     OP_OR = 261,
     OP_XOR = 262,
     OP_AND = 263,
     OP_FNOTEQUAL = 264,
     OP_FEQUAL = 265,
     OP_NOTEQUAL = 266,
     OP_EQUAL = 267,
     OP_FGREATEQUAL = 268,
     OP_FLESSEQUAL = 269,
     OP_FGREAT = 270,
     OP_FLESS = 271,
     OP_SGREAT = 272,
     OP_SLESSEQUAL = 273,
     OP_SGREATEQUAL = 274,
     OP_SLESS = 275,
     OP_LESSEQUAL = 276,
     OP_GREATEQUAL = 277,
     OP_SRIGHT = 278,
     OP_RIGHT = 279,
     OP_LEFT = 280,
     OP_FSUB = 281,
     OP_FADD = 282,
     OP_FDIV = 283,
     OP_FMULT = 284,
     OP_SREM = 285,
     OP_SDIV = 286,
     OP_ZEXT = 287,
     OP_CARRY = 288,
     OP_BORROW = 289,
     OP_SEXT = 290,
     OP_SCARRY = 291,
     OP_SBORROW = 292,
     OP_NAN = 293,
     OP_ABS = 294,
     OP_SQRT = 295,
     OP_CEIL = 296,
     OP_FLOOR = 297,
     OP_ROUND = 298,
     OP_INT2FLOAT = 299,
     OP_FLOAT2FLOAT = 300,
     OP_TRUNC = 301,
     OP_CPOOLREF = 302,
     OP_NEW = 303,
     OP_POPCOUNT = 304,
     BADINTEGER = 305,
     GOTO_KEY = 306,
     CALL_KEY = 307,
     RETURN_KEY = 308,
     IF_KEY = 309,
     DEFINE_KEY = 310,
     ATTACH_KEY = 311,
     MACRO_KEY = 312,
     SPACE_KEY = 313,
     TYPE_KEY = 314,
     RAM_KEY = 315,
     DEFAULT_KEY = 316,
     REGISTER_KEY = 317,
     ENDIAN_KEY = 318,
     WITH_KEY = 319,
     ALIGN_KEY = 320,
     OP_UNIMPL = 321,
     TOKEN_KEY = 322,
     SIGNED_KEY = 323,
     NOFLOW_KEY = 324,
     HEX_KEY = 325,
     DEC_KEY = 326,
     BIG_KEY = 327,
     LITTLE_KEY = 328,
     SIZE_KEY = 329,
     WORDSIZE_KEY = 330,
     OFFSET_KEY = 331,
     NAMES_KEY = 332,
     VALUES_KEY = 333,
     VARIABLES_KEY = 334,
     PCODEOP_KEY = 335,
     IS_KEY = 336,
     LOCAL_KEY = 337,
     DELAYSLOT_KEY = 338,
     CROSSBUILD_KEY = 339,
     EXPORT_KEY = 340,
     BUILD_KEY = 341,
     CONTEXT_KEY = 342,
     ELLIPSIS_KEY = 343,
     GLOBALSET_KEY = 344,
     BITRANGE_KEY = 345,
     CHAR = 346,
     INTEGER = 347,
     INTB = 348,
     STRING = 349,
     SYMBOLSTRING = 350,
     SPACESYM = 351,
     SECTIONSYM = 352,
     TOKENSYM = 353,
     USEROPSYM = 354,
     VALUESYM = 355,
     VALUEMAPSYM = 356,
     CONTEXTSYM = 357,
     NAMESYM = 358,
     VARSYM = 359,
     BITSYM = 360,
     SPECSYM = 361,
     VARLISTSYM = 362,
     OPERANDSYM = 363,
     STARTSYM = 364,
     ENDSYM = 365,
     MACROSYM = 366,
     LABELSYM = 367,
     SUBTABLESYM = 368
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 214 of yacc.c  */
#line 29 "slghparse.y"

  char ch;
  uintb *i;
  intb *big;
  string *str;
  vector<string> *strlist;
  vector<intb> *biglist;
  vector<ExprTree *> *param;
  SpaceQuality *spacequal;
  FieldQuality *fieldqual;
  StarQuality *starqual;
  VarnodeTpl *varnode;
  ExprTree *tree;
  vector<OpTpl *> *stmt;
  ConstructTpl *sem;
  SectionVector *sectionstart;
  Constructor *construct;
  PatternEquation *pateq;
  PatternExpression *patexp;

  vector<SleighSymbol *> *symlist;
  vector<ContextChange *> *contop;
  SleighSymbol *anysym;
  SpaceSymbol *spacesym;
  SectionSymbol *sectionsym;
  TokenSymbol *tokensym;
  UserOpSymbol *useropsym;
  MacroSymbol *macrosym;
  LabelSymbol *labelsym;
  SubtableSymbol *subtablesym;
  StartSymbol *startsym;
  EndSymbol *endsym;
  OperandSymbol *operandsym;
  VarnodeListSymbol *varlistsym;
  VarnodeSymbol *varsym;
  BitrangeSymbol *bitsym;
  NameSymbol *namesym;
  ValueSymbol *valuesym;
  ValueMapSymbol *valuemapsym;
  ContextSymbol *contextsym;
  FamilySymbol *famsym;
  SpecificSymbol *specsym;



/* Line 214 of yacc.c  */
#line 282 "slghparse.cc"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 264 of yacc.c  */
#line 294 "slghparse.cc"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2614

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  137
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  71
/* YYNRULES -- Number of rules.  */
#define YYNRULES  338
/* YYNRULES -- Number of states.  */
#define YYNSTATES  713

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   368

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,   136,    43,     2,     2,     2,    38,    11,     2,
     129,   130,    36,    32,   131,    33,     2,    37,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   135,     8,
      17,   128,    18,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   132,     2,   133,     9,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   134,     6,   127,    44,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     7,    10,    12,    13,    14,    15,    16,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    34,    35,    39,    40,    41,    42,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     5,     8,    11,    14,    16,    18,    20,
      22,    24,    26,    28,    30,    32,    35,    37,    39,    41,
      44,    50,    56,    62,    65,    72,    82,    92,    95,    99,
     102,   106,   109,   117,   125,   128,   131,   134,   142,   150,
     153,   156,   159,   162,   165,   169,   173,   178,   183,   188,
     193,   196,   207,   213,   218,   220,   223,   232,   237,   243,
     249,   255,   260,   267,   269,   272,   275,   278,   279,   281,
     283,   284,   286,   292,   296,   301,   303,   309,   315,   318,
     321,   324,   327,   330,   333,   336,   339,   342,   345,   348,
     350,   353,   355,   357,   359,   363,   367,   371,   375,   379,
     383,   387,   391,   395,   399,   402,   405,   407,   411,   415,
     419,   422,   424,   427,   429,   431,   435,   439,   443,   447,
     451,   455,   459,   463,   465,   467,   469,   471,   472,   476,
     477,   483,   492,   501,   507,   510,   514,   518,   521,   523,
     527,   529,   534,   540,   544,   549,   550,   553,   558,   565,
     570,   576,   581,   589,   596,   600,   606,   612,   622,   627,
     632,   637,   641,   647,   653,   659,   663,   669,   675,   679,
     685,   688,   694,   700,   702,   704,   707,   711,   715,   719,
     723,   727,   731,   735,   739,   743,   747,   751,   755,   759,
     762,   765,   769,   773,   777,   781,   785,   789,   793,   797,
     801,   805,   809,   812,   816,   820,   824,   828,   832,   836,
     840,   844,   848,   852,   856,   860,   864,   867,   872,   877,
     882,   887,   894,   901,   908,   913,   918,   923,   928,   933,
     938,   943,   948,   955,   960,   965,   969,   976,   978,   983,
     988,   995,  1000,  1004,  1006,  1008,  1010,  1012,  1014,  1016,
    1021,  1023,  1025,  1027,  1029,  1031,  1033,  1035,  1037,  1041,
    1044,  1049,  1051,  1053,  1055,  1059,  1063,  1065,  1068,  1073,
    1077,  1079,  1081,  1083,  1085,  1087,  1089,  1091,  1093,  1095,
    1097,  1099,  1101,  1103,  1106,  1110,  1112,  1115,  1117,  1120,
    1122,  1125,  1129,  1132,  1136,  1138,  1140,  1143,  1146,  1150,
    1152,  1154,  1157,  1160,  1164,  1166,  1168,  1170,  1172,  1175,
    1178,  1181,  1185,  1187,  1189,  1191,  1194,  1197,  1198,  1200,
    1204,  1205,  1207,  1211,  1213,  1215,  1217,  1219,  1221,  1223,
    1225,  1227,  1229,  1231,  1233,  1235,  1237,  1239,  1241
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int16 yyrhs[] =
{
     138,     0,    -1,   141,    -1,   138,   142,    -1,   138,   139,
      -1,   138,   140,    -1,   143,    -1,   145,    -1,   149,    -1,
     151,    -1,   152,    -1,   155,    -1,   156,    -1,   157,    -1,
     158,    -1,     1,     8,    -1,   167,    -1,   159,    -1,   162,
      -1,     1,   127,    -1,    68,    76,   128,    85,     8,    -1,
      68,    76,   128,    86,     8,    -1,    68,    78,   128,   105,
       8,    -1,   144,     8,    -1,    68,    80,   107,   129,   105,
     130,    -1,    68,    80,   107,   129,   105,   130,    76,   128,
      86,    -1,    68,    80,   107,   129,   105,   130,    76,   128,
      85,    -1,   144,   147,    -1,    68,    80,   207,    -1,   146,
       8,    -1,    68,   100,   117,    -1,   146,   148,    -1,   107,
     128,   129,   105,   131,   105,   130,    -1,   207,   128,   129,
     105,   131,   105,   130,    -1,   147,    81,    -1,   147,    83,
      -1,   147,    84,    -1,   107,   128,   129,   105,   131,   105,
     130,    -1,   207,   128,   129,   105,   131,   105,   130,    -1,
     148,    81,    -1,   148,    82,    -1,   148,    83,    -1,   148,
      84,    -1,   150,     8,    -1,    68,    71,   107,    -1,    68,
      71,   207,    -1,   150,    72,   128,    73,    -1,   150,    72,
     128,    75,    -1,   150,    87,   128,   105,    -1,   150,    88,
     128,   105,    -1,   150,    74,    -1,    68,   109,    89,   128,
     105,    87,   128,   105,   197,     8,    -1,    68,   109,    89,
     128,    63,    -1,    68,   103,   153,     8,    -1,   154,    -1,
     153,   154,    -1,   107,   128,   117,   132,   105,   131,   105,
     133,    -1,    68,    93,   197,     8,    -1,    69,    91,   201,
     195,     8,    -1,    69,    90,   201,   199,     8,    -1,    69,
      92,   201,   203,     8,    -1,   165,   134,   181,   127,    -1,
      77,   163,   135,   164,   176,   134,    -1,   160,    -1,   161,
     139,    -1,   161,   140,    -1,   161,   127,    -1,    -1,   126,
      -1,   107,    -1,    -1,   171,    -1,    70,   107,   129,   206,
     130,    -1,   134,   181,   127,    -1,   134,   180,   182,   127,
      -1,    79,    -1,   168,    94,   171,   176,   166,    -1,   169,
      94,   171,   176,   166,    -1,   169,   107,    -1,   169,   194,
      -1,   169,   108,    -1,   169,     9,    -1,   168,     9,    -1,
     168,   107,    -1,   168,   194,    -1,   168,   136,    -1,   168,
     108,    -1,   126,   135,    -1,   107,   135,    -1,   135,    -1,
     169,   136,    -1,   106,    -1,   192,    -1,   193,    -1,   129,
     170,   130,    -1,   170,    32,   170,    -1,   170,    33,   170,
      -1,   170,    36,   170,    -1,   170,    31,   170,    -1,   170,
      30,   170,    -1,   170,    12,   170,    -1,   170,     7,   170,
      -1,   170,    10,   170,    -1,   170,    37,   170,    -1,    33,
     170,    -1,    44,   170,    -1,   172,    -1,   171,    11,   171,
      -1,   171,     6,   171,    -1,   171,     8,   171,    -1,   101,
     173,    -1,   173,    -1,   174,   101,    -1,   174,    -1,   175,
      -1,   129,   171,   130,    -1,   192,   128,   170,    -1,   192,
      15,   170,    -1,   192,    17,   170,    -1,   192,    27,   170,
      -1,   192,    18,   170,    -1,   192,    28,   170,    -1,   121,
     128,   170,    -1,   121,    -1,   119,    -1,   192,    -1,   126,
      -1,    -1,   132,   177,   133,    -1,    -1,   177,   115,   128,
     170,     8,    -1,   177,   102,   129,   192,   131,   115,   130,
       8,    -1,   177,   102,   129,   193,   131,   115,   130,     8,
      -1,   177,   121,   128,   170,     8,    -1,   177,   107,    -1,
      31,   107,    30,    -1,    31,   110,    30,    -1,   181,   178,
      -1,   179,    -1,   180,   182,   178,    -1,   182,    -1,   182,
      98,   191,     8,    -1,   182,    98,   185,   189,     8,    -1,
     182,    98,   107,    -1,   182,    98,   185,   107,    -1,    -1,
     182,   183,    -1,   182,    95,   107,     8,    -1,   182,    95,
     107,   135,   105,     8,    -1,   189,   128,   184,     8,    -1,
      95,   107,   128,   184,     8,    -1,   107,   128,   184,     8,
      -1,    95,   107,   135,   105,   128,   184,     8,    -1,   107,
     135,   105,   128,   184,     8,    -1,    95,   193,   128,    -1,
     185,   184,   128,   184,     8,    -1,   112,   129,   205,   130,
       8,    -1,   189,   132,   105,   131,   105,   133,   128,   184,
       8,    -1,   118,   128,   184,     8,    -1,   187,   135,   105,
     128,    -1,   187,   129,   105,   130,    -1,    99,   121,     8,
      -1,    97,   187,   131,   110,     8,    -1,    97,   187,   131,
     107,     8,    -1,    96,   129,   105,   130,     8,    -1,    64,
     186,     8,    -1,    67,   184,    64,   186,     8,    -1,    64,
     132,   184,   133,     8,    -1,    65,   186,     8,    -1,    65,
     132,   184,   133,     8,    -1,    66,     8,    -1,    66,   132,
     184,   133,     8,    -1,   124,   129,   205,   130,     8,    -1,
     190,    -1,   187,    -1,   185,   184,    -1,   129,   184,   130,
      -1,   184,    32,   184,    -1,   184,    33,   184,    -1,   184,
      16,   184,    -1,   184,    15,   184,    -1,   184,    17,   184,
      -1,   184,    28,   184,    -1,   184,    27,   184,    -1,   184,
      18,   184,    -1,   184,    26,   184,    -1,   184,    25,   184,
      -1,   184,    24,   184,    -1,   184,    23,   184,    -1,    33,
     184,    -1,    44,   184,    -1,   184,     9,   184,    -1,   184,
      11,   184,    -1,   184,     6,   184,    -1,   184,    31,   184,
      -1,   184,    30,   184,    -1,   184,    29,   184,    -1,   184,
      36,   184,    -1,   184,    37,   184,    -1,   184,    42,   184,
      -1,   184,    38,   184,    -1,   184,    41,   184,    -1,    43,
     184,    -1,   184,     4,   184,    -1,   184,     5,   184,    -1,
     184,     3,   184,    -1,   184,    14,   184,    -1,   184,    13,
     184,    -1,   184,    22,   184,    -1,   184,    21,   184,    -1,
     184,    20,   184,    -1,   184,    19,   184,    -1,   184,    35,
     184,    -1,   184,    34,   184,    -1,   184,    40,   184,    -1,
     184,    39,   184,    -1,    34,   184,    -1,    52,   129,   184,
     130,    -1,    53,   129,   184,   130,    -1,    48,   129,   184,
     130,    -1,    45,   129,   184,   130,    -1,    46,   129,   184,
     131,   184,   130,    -1,    49,   129,   184,   131,   184,   130,
      -1,    50,   129,   184,   131,   184,   130,    -1,    58,   129,
     184,   130,    -1,    57,   129,   184,   130,    -1,    51,   129,
     184,   130,    -1,    59,   129,   184,   130,    -1,    54,   129,
     184,   130,    -1,    55,   129,   184,   130,    -1,    56,   129,
     184,   130,    -1,    61,   129,   184,   130,    -1,    61,   129,
     184,   131,   184,   130,    -1,    62,   129,   184,   130,    -1,
     193,   129,   188,   130,    -1,   193,   135,   105,    -1,   193,
     132,   105,   131,   105,   133,    -1,   118,    -1,   112,   129,
     205,   130,    -1,    60,   129,   205,   130,    -1,    36,   132,
     109,   133,   135,   105,    -1,    36,   132,   109,   133,    -1,
      36,   135,   105,    -1,    36,    -1,   122,    -1,   123,    -1,
     105,    -1,    63,    -1,   121,    -1,   105,   132,   109,   133,
      -1,   190,    -1,   107,    -1,   193,    -1,   188,    -1,   107,
      -1,   126,    -1,   105,    -1,    63,    -1,   105,   135,   105,
      -1,    11,   187,    -1,    11,   135,   105,   187,    -1,   193,
      -1,   107,    -1,   126,    -1,    17,   125,    18,    -1,    17,
     107,    18,    -1,   193,    -1,    11,   187,    -1,    11,   135,
     105,   187,    -1,   105,   135,   105,    -1,   107,    -1,   126,
      -1,   113,    -1,   114,    -1,   115,    -1,   116,    -1,   120,
      -1,   117,    -1,   119,    -1,   121,    -1,   122,    -1,   123,
      -1,   104,    -1,   194,   104,    -1,   132,   196,   133,    -1,
     105,    -1,    33,   105,    -1,   105,    -1,    33,   105,    -1,
     107,    -1,   196,   105,    -1,   196,    33,   105,    -1,   196,
     107,    -1,   132,   198,   133,    -1,   107,    -1,   107,    -1,
     198,   107,    -1,   198,   207,    -1,   132,   200,   133,    -1,
     107,    -1,   207,    -1,   200,   107,    -1,   200,   207,    -1,
     132,   202,   133,    -1,   113,    -1,   115,    -1,   113,    -1,
     115,    -1,   202,   113,    -1,   202,   115,    -1,   202,   107,
      -1,   132,   204,   133,    -1,   117,    -1,   117,    -1,   107,
      -1,   204,   117,    -1,   204,   107,    -1,    -1,   184,    -1,
     205,   131,   184,    -1,    -1,   107,    -1,   206,   131,   107,
      -1,   109,    -1,   110,    -1,   111,    -1,   112,    -1,   124,
      -1,   126,    -1,   113,    -1,   114,    -1,   115,    -1,   116,
      -1,   117,    -1,   120,    -1,   121,    -1,   122,    -1,   123,
      -1,   118,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   156,   156,   157,   158,   159,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   172,   173,   174,   175,
     177,   178,   180,   182,   184,   185,   186,   187,   188,   190,
     192,   193,   196,   197,   198,   199,   200,   202,   203,   204,
     205,   206,   207,   209,   211,   212,   213,   214,   215,   216,
     217,   219,   221,   223,   225,   226,   228,   231,   233,   235,
     237,   239,   242,   244,   245,   246,   248,   250,   251,   252,
     255,   256,   259,   261,   262,   263,   265,   266,   268,   269,
     270,   271,   272,   273,   274,   275,   276,   278,   279,   280,
     281,   283,   285,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   302,   303,   304,   305,
     307,   308,   310,   311,   313,   314,   316,   317,   318,   319,
     320,   321,   322,   325,   326,   327,   328,   330,   331,   333,
     334,   335,   336,   337,   338,   340,   341,   343,   345,   346,
     348,   349,   350,   351,   352,   354,   355,   356,   357,   359,
     360,   361,   362,   363,   364,   365,   366,   367,   368,   369,
     370,   371,   372,   373,   374,   375,   376,   377,   378,   379,
     380,   381,   382,   383,   385,   386,   387,   388,   389,   390,
     391,   392,   393,   394,   395,   396,   397,   398,   399,   400,
     401,   402,   403,   404,   405,   406,   407,   408,   409,   410,
     411,   412,   413,   414,   415,   416,   417,   418,   419,   420,
     421,   422,   423,   424,   425,   426,   427,   428,   429,   430,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     452,   453,   454,   455,   457,   458,   459,   460,   461,   462,
     463,   464,   466,   467,   468,   469,   471,   472,   473,   474,
     475,   477,   478,   479,   481,   482,   484,   485,   486,   487,
     488,   489,   491,   492,   493,   494,   495,   497,   498,   499,
     500,   501,   503,   504,   506,   507,   508,   510,   511,   512,
     514,   515,   516,   519,   520,   522,   523,   524,   526,   528,
     529,   530,   531,   533,   534,   535,   537,   538,   539,   540,
     541,   543,   544,   546,   547,   549,   550,   553,   554,   555,
     557,   558,   559,   561,   562,   563,   564,   565,   566,   567,
     568,   569,   570,   571,   572,   573,   574,   575,   576
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "OP_BOOL_OR", "OP_BOOL_XOR",
  "OP_BOOL_AND", "'|'", "OP_OR", "';'", "'^'", "OP_XOR", "'&'", "OP_AND",
  "OP_FNOTEQUAL", "OP_FEQUAL", "OP_NOTEQUAL", "OP_EQUAL", "'<'", "'>'",
  "OP_FGREATEQUAL", "OP_FLESSEQUAL", "OP_FGREAT", "OP_FLESS", "OP_SGREAT",
  "OP_SLESSEQUAL", "OP_SGREATEQUAL", "OP_SLESS", "OP_LESSEQUAL",
  "OP_GREATEQUAL", "OP_SRIGHT", "OP_RIGHT", "OP_LEFT", "'+'", "'-'",
  "OP_FSUB", "OP_FADD", "'*'", "'/'", "'%'", "OP_FDIV", "OP_FMULT",
  "OP_SREM", "OP_SDIV", "'!'", "'~'", "OP_ZEXT", "OP_CARRY", "OP_BORROW",
  "OP_SEXT", "OP_SCARRY", "OP_SBORROW", "OP_NAN", "OP_ABS", "OP_SQRT",
  "OP_CEIL", "OP_FLOOR", "OP_ROUND", "OP_INT2FLOAT", "OP_FLOAT2FLOAT",
  "OP_TRUNC", "OP_CPOOLREF", "OP_NEW", "OP_POPCOUNT", "BADINTEGER",
  "GOTO_KEY", "CALL_KEY", "RETURN_KEY", "IF_KEY", "DEFINE_KEY",
  "ATTACH_KEY", "MACRO_KEY", "SPACE_KEY", "TYPE_KEY", "RAM_KEY",
  "DEFAULT_KEY", "REGISTER_KEY", "ENDIAN_KEY", "WITH_KEY", "ALIGN_KEY",
  "OP_UNIMPL", "TOKEN_KEY", "SIGNED_KEY", "NOFLOW_KEY", "HEX_KEY",
  "DEC_KEY", "BIG_KEY", "LITTLE_KEY", "SIZE_KEY", "WORDSIZE_KEY",
  "OFFSET_KEY", "NAMES_KEY", "VALUES_KEY", "VARIABLES_KEY", "PCODEOP_KEY",
  "IS_KEY", "LOCAL_KEY", "DELAYSLOT_KEY", "CROSSBUILD_KEY", "EXPORT_KEY",
  "BUILD_KEY", "CONTEXT_KEY", "ELLIPSIS_KEY", "GLOBALSET_KEY",
  "BITRANGE_KEY", "CHAR", "INTEGER", "INTB", "STRING", "SYMBOLSTRING",
  "SPACESYM", "SECTIONSYM", "TOKENSYM", "USEROPSYM", "VALUESYM",
  "VALUEMAPSYM", "CONTEXTSYM", "NAMESYM", "VARSYM", "BITSYM", "SPECSYM",
  "VARLISTSYM", "OPERANDSYM", "STARTSYM", "ENDSYM", "MACROSYM", "LABELSYM",
  "SUBTABLESYM", "'}'", "'='", "'('", "')'", "','", "'['", "']'", "'{'",
  "':'", "' '", "$accept", "spec", "definition", "constructorlike",
  "endiandef", "aligndef", "tokendef", "tokenprop", "contextdef",
  "contextprop", "fielddef", "contextfielddef", "spacedef", "spaceprop",
  "varnodedef", "bitrangedef", "bitrangelist", "bitrangesingle",
  "pcodeopdef", "valueattach", "nameattach", "varattach", "macrodef",
  "withblockstart", "withblockmid", "withblock", "id_or_nil",
  "bitpat_or_nil", "macrostart", "rtlbody", "constructor",
  "constructprint", "subtablestart", "pexpression", "pequation", "elleq",
  "ellrt", "atomic", "constraint", "contextblock", "contextlist",
  "section_def", "rtlfirstsection", "rtlcontinue", "rtl", "rtlmid",
  "statement", "expr", "sizedstar", "jumpdest", "varnode",
  "integervarnode", "lhsvarnode", "label", "exportvarnode", "familysymbol",
  "specificsymbol", "charstring", "intblist", "intbpart", "stringlist",
  "stringpart", "anystringlist", "anystringpart", "valuelist", "valuepart",
  "varlist", "varpart", "paramlist", "oplist", "anysymbol", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   124,   261,    59,    94,
     262,    38,   263,   264,   265,   266,   267,    60,    62,   268,
     269,   270,   271,   272,   273,   274,   275,   276,   277,   278,
     279,   280,    43,    45,   281,   282,    42,    47,    37,   283,
     284,   285,   286,    33,   126,   287,   288,   289,   290,   291,
     292,   293,   294,   295,   296,   297,   298,   299,   300,   301,
     302,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   313,   314,   315,   316,   317,   318,   319,   320,   321,
     322,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   333,   334,   335,   336,   337,   338,   339,   340,   341,
     342,   343,   344,   345,   346,   347,   348,   349,   350,   351,
     352,   353,   354,   355,   356,   357,   358,   359,   360,   361,
     362,   363,   364,   365,   366,   367,   368,   125,    61,    40,
      41,    44,    91,    93,   123,    58,    32
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   137,   138,   138,   138,   138,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   140,   140,   140,   140,
     141,   141,   142,   143,   144,   144,   144,   144,   144,   145,
     146,   146,   147,   147,   147,   147,   147,   148,   148,   148,
     148,   148,   148,   149,   150,   150,   150,   150,   150,   150,
     150,   151,   151,   152,   153,   153,   154,   155,   156,   157,
     158,   159,   160,   161,   161,   161,   162,   163,   163,   163,
     164,   164,   165,   166,   166,   166,   167,   167,   168,   168,
     168,   168,   168,   168,   168,   168,   168,   169,   169,   169,
     169,   170,   170,   170,   170,   170,   170,   170,   170,   170,
     170,   170,   170,   170,   170,   170,   171,   171,   171,   171,
     172,   172,   173,   173,   174,   174,   175,   175,   175,   175,
     175,   175,   175,   175,   175,   175,   175,   176,   176,   177,
     177,   177,   177,   177,   177,   178,   178,   179,   180,   180,
     181,   181,   181,   181,   181,   182,   182,   182,   182,   183,
     183,   183,   183,   183,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   183,   183,   183,   183,   183,   183,   183,
     183,   183,   183,   183,   184,   184,   184,   184,   184,   184,
     184,   184,   184,   184,   184,   184,   184,   184,   184,   184,
     184,   184,   184,   184,   184,   184,   184,   184,   184,   184,
     184,   184,   184,   184,   184,   184,   184,   184,   184,   184,
     184,   184,   184,   184,   184,   184,   184,   184,   184,   184,
     184,   184,   184,   184,   184,   184,   184,   184,   184,   184,
     184,   184,   184,   184,   184,   184,   184,   184,   184,   184,
     185,   185,   185,   185,   186,   186,   186,   186,   186,   186,
     186,   186,   187,   187,   187,   187,   188,   188,   188,   188,
     188,   189,   189,   189,   190,   190,   191,   191,   191,   191,
     191,   191,   192,   192,   192,   192,   192,   193,   193,   193,
     193,   193,   194,   194,   195,   195,   195,   196,   196,   196,
     196,   196,   196,   197,   197,   198,   198,   198,   199,   200,
     200,   200,   200,   201,   201,   201,   202,   202,   202,   202,
     202,   203,   203,   204,   204,   204,   204,   205,   205,   205,
     206,   206,   206,   207,   207,   207,   207,   207,   207,   207,
     207,   207,   207,   207,   207,   207,   207,   207,   207
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     2,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     1,     1,     1,     2,
       5,     5,     5,     2,     6,     9,     9,     2,     3,     2,
       3,     2,     7,     7,     2,     2,     2,     7,     7,     2,
       2,     2,     2,     2,     3,     3,     4,     4,     4,     4,
       2,    10,     5,     4,     1,     2,     8,     4,     5,     5,
       5,     4,     6,     1,     2,     2,     2,     0,     1,     1,
       0,     1,     5,     3,     4,     1,     5,     5,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     1,
       2,     1,     1,     1,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     2,     2,     1,     3,     3,     3,
       2,     1,     2,     1,     1,     3,     3,     3,     3,     3,
       3,     3,     3,     1,     1,     1,     1,     0,     3,     0,
       5,     8,     8,     5,     2,     3,     3,     2,     1,     3,
       1,     4,     5,     3,     4,     0,     2,     4,     6,     4,
       5,     4,     7,     6,     3,     5,     5,     9,     4,     4,
       4,     3,     5,     5,     5,     3,     5,     5,     3,     5,
       2,     5,     5,     1,     1,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     2,
       2,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     4,     4,     4,
       4,     6,     6,     6,     4,     4,     4,     4,     4,     4,
       4,     4,     6,     4,     4,     3,     6,     1,     4,     4,
       6,     4,     3,     1,     1,     1,     1,     1,     1,     4,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     2,
       4,     1,     1,     1,     3,     3,     1,     2,     4,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     3,     1,     2,     1,     2,     1,
       2,     3,     2,     3,     1,     1,     2,     2,     3,     1,
       1,     2,     2,     3,     1,     1,     1,     1,     2,     2,
       2,     3,     1,     1,     1,     2,     2,     0,     1,     3,
       0,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
       0,     0,     0,     2,     0,     1,     0,     0,     0,     0,
      67,     0,     0,    89,     4,     5,     3,     6,     0,     7,
       0,     8,     0,     9,    10,    11,    12,    13,    14,    17,
      63,     0,    18,     0,    16,     0,     0,     0,    15,    19,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    69,    68,     0,    88,    87,    23,     0,   323,   324,
     325,   326,   329,   330,   331,   332,   333,   338,   334,   335,
     336,   337,   327,   328,    27,     0,    29,     0,    31,     0,
      43,     0,    50,     0,     0,     0,    66,    64,    65,   145,
      82,     0,   282,    83,    86,    85,    84,    81,     0,    78,
      80,    90,    79,     0,     0,    44,    45,     0,     0,    28,
     294,     0,     0,    30,     0,     0,    54,     0,   304,   305,
       0,     0,     0,     0,   320,    70,     0,    34,    35,    36,
       0,     0,    39,    40,    41,    42,     0,     0,     0,     0,
       0,   140,     0,   272,   273,   274,   275,   124,   276,   123,
     126,     0,   127,   106,   111,   113,   114,   125,   283,   127,
      20,    21,     0,     0,   295,     0,    57,     0,    53,    55,
       0,   306,   307,     0,     0,     0,     0,   285,     0,     0,
     312,     0,     0,   321,     0,   127,    71,     0,     0,     0,
       0,    46,    47,    48,    49,    61,     0,     0,   243,   257,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   256,
     254,     0,   277,     0,   278,   279,   280,   281,     0,   255,
     146,     0,     0,   253,     0,   173,   252,   110,     0,     0,
       0,     0,     0,   129,     0,   112,     0,     0,     0,     0,
       0,     0,     0,    22,     0,   296,   293,   297,     0,    52,
       0,   310,   308,   309,   303,   299,     0,   300,    59,   286,
       0,   287,   289,     0,    58,   314,   313,     0,    60,    72,
       0,     0,     0,     0,     0,     0,   254,   255,     0,   259,
     252,     0,     0,     0,     0,   247,   246,   251,   248,   244,
     245,     0,     0,   250,     0,     0,   170,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     237,     0,     0,     0,   174,   252,     0,     0,     0,     0,
       0,     0,   143,   271,     0,     0,   266,     0,     0,     0,
       0,   317,     0,   317,     0,     0,     0,     0,     0,     0,
       0,    91,     0,   122,    92,    93,   115,   108,   109,   107,
       0,    75,   145,    76,   117,   118,   120,   119,   121,   116,
      77,    24,     0,     0,   301,   298,   302,   288,     0,   290,
     292,   284,   316,   315,   311,   322,    62,     0,     0,     0,
       0,     0,   265,   264,     0,   242,     0,     0,   165,     0,
     168,     0,   189,   216,   202,   190,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     317,     0,     0,   317,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   175,     0,     0,     0,   147,     0,     0,   154,
       0,     0,     0,   267,     0,   144,   263,     0,   261,   141,
     161,   258,     0,     0,   318,     0,     0,     0,     0,     0,
       0,     0,     0,   104,   105,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   134,     0,     0,   128,
     138,   145,     0,     0,     0,     0,   291,     0,     0,     0,
       0,   260,   241,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   176,   205,   203,   204,   193,
     191,   192,   207,   206,   180,   179,   181,   184,   211,   210,
     209,   208,   188,   187,   186,   185,   183,   182,   196,   195,
     194,   177,   178,   213,   212,   197,   198,   200,   215,   214,
     201,   199,     0,     0,     0,   235,     0,     0,     0,     0,
       0,     0,   269,   142,   151,     0,     0,     0,   158,     0,
       0,   160,   159,   149,     0,    94,   101,   102,   100,    99,
      98,    95,    96,    97,   103,     0,     0,     0,     0,     0,
      73,   137,     0,     0,     0,    32,    33,    37,    38,     0,
     249,   167,   169,   171,   220,     0,   219,     0,     0,   226,
     217,   218,   228,   229,   230,   225,   224,   227,   239,   231,
       0,   233,   238,   166,   234,     0,   150,   148,     0,   164,
     163,   162,   268,     0,   156,   319,   172,   155,     0,     0,
       0,     0,     0,    74,   139,     0,     0,    26,    25,     0,
       0,   240,     0,     0,     0,     0,     0,     0,   153,     0,
       0,     0,   130,   133,   135,   136,    56,    51,   221,   222,
     223,   232,   236,   152,     0,     0,     0,     0,     0,     0,
     157,   131,   132
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,    14,    15,     3,    16,    17,    18,    19,    20,
      74,    78,    21,    22,    23,    24,   115,   116,    25,    26,
      27,    28,    29,    30,    31,    32,    53,   185,    33,   363,
      34,    35,    36,   353,   152,   153,   154,   155,   156,   234,
     360,   621,   510,   511,   140,   141,   220,   484,   323,   292,
     324,   223,   224,   293,   335,   354,   325,    96,   179,   263,
     112,   165,   175,   256,   121,   173,   182,   267,   485,   184,
      75
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -245
static const yytype_int16 yypact[] =
{
      24,    22,     3,  -245,  -102,  -245,     5,   -19,   375,   -30,
     -76,   -15,    -1,  -245,  -245,  -245,  -245,  -245,   388,  -245,
     419,  -245,    21,  -245,  -245,  -245,  -245,  -245,  -245,  -245,
    -245,    37,  -245,   -21,  -245,    23,    87,   -45,  -245,  -245,
    2452,    32,  2470,   -74,    56,    71,   108,    33,    33,    33,
      73,  -245,  -245,   132,  -245,  -245,  -245,   144,  -245,  -245,
    -245,  -245,  -245,  -245,  -245,  -245,  -245,  -245,  -245,  -245,
    -245,  -245,  -245,  -245,   351,   159,  -245,   189,   291,   200,
    -245,   252,  -245,   254,   256,  1459,  -245,  -245,  -245,  -245,
    -245,  2327,  -245,  -245,  -245,  -245,   293,  -245,  2327,  -245,
    -245,  -245,   293,   383,   393,  -245,  -245,   299,   277,  -245,
    -245,   301,   417,  -245,   308,     8,  -245,   309,  -245,  -245,
     170,   320,   -14,   -78,   324,  2327,   310,  -245,  -245,  -245,
     331,   333,  -245,  -245,  -245,  -245,   334,   219,   350,   363,
     342,  1662,  1743,  -245,  -245,  -245,  -245,  -245,  -245,   344,
    -245,  2327,     4,  -245,  -245,   369,  -245,   251,  -245,     4,
    -245,  -245,   474,   381,  -245,  2348,  -245,   367,  -245,  -245,
     -41,  -245,  -245,   167,  2488,   482,   389,  -245,    16,   485,
    -245,   -42,   488,  -245,   -44,   384,   278,   408,   410,   412,
     413,  -245,  -245,  -245,  -245,  -245,   264,   -59,   141,  -245,
     271,  1668,     1,  1530,   288,   390,  1561,   366,   399,   386,
      39,   394,  -245,   397,  -245,  -245,  -245,  -245,   398,   -50,
    -245,  1530,   -67,  -245,    29,  -245,    34,  -245,  1689,    17,
    2327,  2327,  2327,  -245,   -58,  -245,  1689,  1689,  1689,  1689,
    1689,  1689,   -58,  -245,   392,  -245,  -245,  -245,   425,  -245,
     457,  -245,  -245,  -245,  -245,  -245,  2373,  -245,  -245,  -245,
     453,  -245,  -245,   -22,  -245,  -245,  -245,   -80,  -245,  -245,
     452,   426,   430,   439,   440,   442,  -245,  -245,   470,  -245,
    -245,   592,   593,   504,   510,  -245,   518,  -245,  -245,  -245,
    -245,  1530,   645,  -245,  1530,   647,  -245,  1530,  1530,  1530,
    1530,  1530,   522,   557,   562,   565,   602,   605,   638,   639,
     649,   680,   685,   688,   690,   725,   728,   730,   765,   770,
    -245,  1530,  1810,  1530,  -245,   -10,     0,   564,   627,   764,
     307,   641,   926,  -245,  1548,   931,  -245,   966,   830,  1530,
     870,  1530,  1530,  1530,  1487,   874,   909,  1530,   910,  1689,
    1689,  -245,  1689,  2360,  -245,  -245,  -245,   290,  1008,  -245,
      75,  -245,  -245,  -245,  2360,  2360,  2360,  2360,  2360,  2360,
    -245,   978,   950,   971,  -245,  -245,  -245,  -245,   954,  -245,
    -245,  -245,  -245,  -245,  -245,  -245,  -245,   989,   990,  1029,
    1030,  1561,  -245,  -245,  1006,  -245,  1065,   326,  -245,   563,
    -245,   603,  -245,  -245,  -245,  -245,  1530,  1530,  1530,  1530,
    1530,  1530,  1530,  1530,  1530,  1530,  1530,  1530,  1530,  1530,
    1530,  1530,  1530,  1530,   807,  1530,  1530,  1530,  1530,  1530,
    1530,  1530,  1530,  1530,  1530,  1530,  1530,  1530,  1530,  1530,
    1530,  1530,  1530,  1530,  1530,  1530,  1530,  1530,  1530,  1530,
    1530,  1530,  1530,  1530,  1530,  1530,  1530,  1530,  1530,  1530,
    1530,  1627,  -245,     7,  1070,  1074,  -245,  1530,  1109,  -245,
    1085,    42,  1114,  -245,  1149,  1251,  -245,  1286,  -245,  -245,
    -245,  -245,  1862,  1171,  2182,    62,  1902,    76,  1530,  1125,
    1206,  1942,  1164,  -245,  -245,   283,  1689,  1689,  1689,  1689,
    1689,  1689,  1689,  1689,  1689,  1210,  -245,  1246,  1291,  -245,
    -245,  -245,   -11,  1326,  1204,  1270,  -245,  1249,  1284,  1285,
    1325,  -245,  1324,  1361,  1491,  1525,  1527,   847,   684,   887,
     724,   766,   927,   967,  1007,  1047,  1087,  1127,  1167,  1207,
    1247,    96,   643,  1287,   181,  -245,  2221,  2258,  2258,  2292,
    2324,  2385,  2490,  2490,  2490,  2490,  2516,  2516,  2516,  2516,
    2516,  2516,  2516,  2516,  2516,  2516,  2516,  2516,   514,   514,
     514,   379,   379,   379,   379,  -245,  -245,  -245,  -245,  -245,
    -245,  -245,  1532,  1365,  1411,  -245,  1982,     9,  1535,  1536,
    1537,  1561,  -245,  -245,  -245,  1530,  1538,  1530,  -245,  1539,
    2022,  -245,  -245,  -245,  1433,  -245,   444,  1705,   168,   289,
     289,   287,   287,  -245,  -245,  1490,  1689,  1689,  1597,   226,
    -245,  -245,   303,  1443,   -74,  -245,  -245,  -245,  -245,  1444,
    -245,  -245,  -245,  -245,  -245,  1530,  -245,  1530,  1530,  -245,
    -245,  -245,  -245,  -245,  -245,  -245,  -245,  -245,  -245,  -245,
    1530,  -245,  -245,  -245,  -245,  1445,  -245,  -245,  1530,  -245,
    -245,  -245,  -245,  2062,  -245,  2182,  -245,  -245,  1418,  1422,
    1427,  1524,  1589,  -245,  -245,  1540,  1541,  -245,  -245,  1432,
    1559,  -245,  1327,  1367,  1407,  1447,  1436,  2102,  -245,  1466,
    1480,  1483,  -245,  -245,  -245,  -245,  -245,  -245,  -245,  -245,
    -245,  -245,  -245,  -245,  1530,  1470,  1472,  2142,  1608,  1609,
    -245,  -245,  -245
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -245,  -245,  1587,  1592,  -245,  -245,  -245,  -245,  -245,  -245,
    -245,  -245,  -245,  -245,  -245,  -245,  -245,  1512,  -245,  -245,
    -245,  -245,  -245,  -245,  -245,  -245,  -245,  -245,  -245,  1387,
    -245,  -245,  -245,  -194,   -62,  -245,  1488,  -245,  -245,  -129,
    -245,  1013,  -245,  -245,  1272,  1121,  -245,  -197,  -140,  -196,
    -127,  1173,  1304,  -139,  -245,   -91,   -53,  1603,  -245,  -245,
    1016,  -245,  -245,  -245,   409,  -245,  -245,  -245,  -244,  -245,
      15
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -271
static const yytype_int16 yytable[] =
{
     157,   221,   225,     5,     6,   295,   322,   157,   466,   296,
     230,   378,   231,    38,   222,   232,   168,   657,   196,   176,
     619,   361,   249,   230,   344,   231,    37,   382,   232,    80,
     242,    51,    90,   110,   157,    79,   159,   383,     6,   180,
     103,   104,   364,   365,   366,   367,   368,   369,   281,   260,
      52,   157,    40,   384,   181,   106,   271,   109,   111,    41,
     157,    42,   345,   186,   250,   265,   282,   334,   346,   279,
     199,     7,     8,     9,    43,   266,   362,    50,  -263,   329,
      10,    44,  -263,   379,    45,   380,   269,   270,   226,   229,
      46,   177,     1,    81,   397,    82,    97,   399,     4,   487,
     401,   402,   403,   404,   405,    85,     8,     9,    83,    84,
      11,   381,   209,    89,    10,   114,   620,    91,   178,   463,
      54,   261,   464,   262,   424,   465,   462,    92,   467,    12,
      93,    94,    39,   297,    55,   468,   233,   658,    13,   157,
     157,   157,   482,   280,    11,   486,   118,   356,   119,   589,
     491,   327,   590,   280,   336,   493,   494,   347,   495,    95,
     107,   348,  -261,    12,    86,   120,  -261,   339,   357,   358,
     359,  -262,    13,   113,   340,   355,   541,   505,   114,   544,
     247,    98,   506,   355,   355,   355,   355,   355,   355,   257,
     507,    92,   596,   597,    99,   100,   508,   117,   499,   500,
     501,   502,   124,   473,   503,   504,   599,   597,   509,   527,
     528,   529,   530,   531,   532,   533,   534,   535,   536,   537,
     538,   539,   540,   101,   542,   543,   648,   597,   546,   547,
     548,   549,   550,   551,   552,   553,   554,   555,   556,   557,
     558,   559,   560,   561,   562,   563,   564,   565,   566,   567,
     568,   569,   570,   571,   572,   573,   574,   575,   576,   577,
     578,   579,   580,   581,   521,   582,   236,   125,   237,   238,
     586,   376,   126,   283,   251,   196,   284,   280,   239,   240,
     252,   478,   253,   171,   230,   172,   231,   130,   197,   232,
     496,   600,   191,   497,   192,   498,   355,   355,   231,   355,
     254,   232,   606,   607,   608,   609,   610,   611,   612,   613,
     614,   652,   597,   499,   500,   501,   502,   131,   196,   503,
     504,   501,   502,   503,   504,   503,   504,   199,   136,   425,
     426,   427,   428,   675,   285,   429,   676,   430,   280,   431,
     432,   433,   434,   435,   436,   437,   438,   439,   440,   441,
     442,   443,   444,   445,   446,   447,   448,   449,   450,   451,
     452,   453,   454,   455,   456,   457,   458,   459,   460,   209,
     199,   276,   132,   133,   134,   135,   286,   330,   287,   241,
     137,   212,   138,   214,   139,   215,   216,   217,   677,   678,
     277,   160,   288,   289,   290,   326,    56,   158,   663,   278,
     665,   161,   198,   291,   162,   212,   163,   214,   164,   215,
     216,   217,   209,   605,   276,   454,   455,   456,   457,   458,
     459,   460,   671,   672,   212,   166,   214,    76,   215,   216,
     217,   183,   127,   277,   128,   129,   167,   170,   682,   187,
     683,   684,   472,   355,   355,   355,   355,   355,   355,   355,
     355,   355,   174,   685,   497,   193,   498,   122,   123,   524,
     188,   687,   189,   190,   662,    47,    48,    49,   194,   195,
     235,   331,   228,   332,   499,   500,   501,   502,   221,   225,
     503,   504,   243,   212,   248,   214,   244,   215,   216,   217,
     258,   222,   333,   264,   259,    57,   268,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,   707,    68,    69,
      70,    71,    72,   272,    73,   273,   233,   274,   275,   328,
     337,   338,   371,   341,   669,   342,    77,   343,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,   280,    68,
      69,    70,    71,    72,   373,    73,   450,   451,   452,   453,
     454,   455,   456,   457,   458,   459,   460,   372,   377,   385,
     386,   387,   670,   355,   355,   226,   425,   426,   427,   428,
     388,   389,   429,   390,   430,   391,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,   456,   457,   458,   459,   460,   425,   426,   427,   428,
     392,   393,   429,   394,   430,   395,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,   456,   457,   458,   459,   460,   425,   426,   427,   428,
     396,   406,   429,   398,   430,   400,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     455,   456,   457,   458,   459,   460,   407,   425,   426,   427,
     428,   408,   469,   429,   409,   430,   525,   431,   432,   433,
     434,   435,   436,   437,   438,   439,   440,   441,   442,   443,
     444,   445,   446,   447,   448,   449,   450,   451,   452,   453,
     454,   455,   456,   457,   458,   459,   460,   425,   426,   427,
     428,   410,   470,   429,   411,   430,   526,   431,   432,   433,
     434,   435,   436,   437,   438,   439,   440,   441,   442,   443,
     444,   445,   446,   447,   448,   449,   450,   451,   452,   453,
     454,   455,   456,   457,   458,   459,   460,   412,   413,   425,
     426,   427,   428,   649,   650,   429,   474,   430,   414,   431,
     432,   433,   434,   435,   436,   437,   438,   439,   440,   441,
     442,   443,   444,   445,   446,   447,   448,   449,   450,   451,
     452,   453,   454,   455,   456,   457,   458,   459,   460,   415,
     425,   426,   427,   428,   416,   635,   429,   417,   430,   418,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   419,   637,   429,   420,   430,   421,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   422,   471,   429,   638,   430,   423,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,  -270,   481,   429,   545,   430,   479,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   480,   483,   429,   634,   430,   489,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   490,   492,   429,   636,   430,   232,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   513,   514,   429,   639,   430,   516,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   517,   518,   429,   640,   430,   515,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   519,   520,   429,   641,   430,   522,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   523,   584,   429,   642,   430,   585,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   587,   588,   429,   643,   430,   591,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   592,   601,   429,   644,   430,  -262,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   593,   604,   429,   645,   430,   595,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   602,   623,   429,   646,   430,   615,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   616,   624,   429,   647,   430,   625,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   626,   627,   429,   651,   430,   617,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   622,   628,   429,   698,   430,   629,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
     425,   426,   427,   428,   630,   654,   429,   699,   430,   631,
     431,   432,   433,   434,   435,   436,   437,   438,   439,   440,
     441,   442,   443,   444,   445,   446,   447,   448,   449,   450,
     451,   452,   453,   454,   455,   456,   457,   458,   459,   460,
      40,   496,   692,   632,   497,   633,   498,   700,   668,    42,
     653,   196,   655,   659,   660,   661,   664,   666,   679,   681,
     686,   689,    43,   690,   499,   500,   501,   502,   691,    44,
     503,   504,    45,   298,   299,   696,   198,   697,    46,   702,
     694,   695,   196,   300,   301,   302,   303,   701,   304,   305,
     306,   307,   308,   309,   310,   311,   312,   313,   314,   315,
     316,   317,   318,   199,   704,   705,   496,   693,   706,   497,
     708,   498,   709,   143,   144,   145,   146,   212,   196,   214,
     148,   215,   216,   217,   197,   488,   711,   712,    87,   499,
     500,   501,   502,    88,   199,   503,   504,   169,   619,   370,
     227,   674,   618,   198,   512,   209,   583,   276,   477,   102,
     680,     0,   319,     0,   197,     0,     0,   212,   320,   214,
       0,   215,   216,   217,     0,   475,   277,     0,     0,   321,
     199,   200,   201,   202,   203,   212,   209,   214,   276,   215,
     216,   217,     0,   196,   476,     0,     0,     0,   212,   197,
     214,     0,   215,   216,   217,   197,     0,   277,     0,     0,
     285,     0,   204,   205,   206,     0,   208,     0,   198,     0,
       0,     0,   209,     0,   210,     0,     0,     0,     0,   211,
       0,     0,     0,     0,   212,   213,   214,   498,   215,   216,
     217,   218,   349,   219,   673,   199,   200,   201,   202,   203,
       0,   285,   286,   350,   287,   499,   500,   501,   502,     0,
       0,   503,   504,     0,     0,     0,     0,     0,   288,   289,
     290,     0,     0,     0,     0,     0,     0,   204,   205,   206,
     207,   208,     0,     0,     0,     0,     0,   209,     0,   210,
       0,     0,     0,   286,   211,   287,     0,     0,     0,   212,
     213,   214,     0,   215,   216,   217,   218,     0,   219,   288,
     289,   290,     0,     0,     0,   351,     0,     0,     0,     0,
     294,     0,   143,   144,   145,   146,   212,     0,   214,   148,
     215,   216,   217,   425,   426,   427,   428,     0,   352,   429,
       0,   430,     0,   431,   432,   433,   434,   435,   436,   437,
     438,   439,   440,   441,   442,   443,   444,   445,   446,   447,
     448,   449,   450,   451,   452,   453,   454,   455,   456,   457,
     458,   459,   460,     0,     0,     0,   143,   144,   145,   146,
       0,     0,   147,   148,   149,   425,   426,   427,   428,   150,
     594,   429,   151,   430,   461,   431,   432,   433,   434,   435,
     436,   437,   438,   439,   440,   441,   442,   443,   444,   445,
     446,   447,   448,   449,   450,   451,   452,   453,   454,   455,
     456,   457,   458,   459,   460,   425,   426,   427,   428,     0,
     598,   429,     0,   430,     0,   431,   432,   433,   434,   435,
     436,   437,   438,   439,   440,   441,   442,   443,   444,   445,
     446,   447,   448,   449,   450,   451,   452,   453,   454,   455,
     456,   457,   458,   459,   460,   425,   426,   427,   428,     0,
     603,   429,     0,   430,     0,   431,   432,   433,   434,   435,
     436,   437,   438,   439,   440,   441,   442,   443,   444,   445,
     446,   447,   448,   449,   450,   451,   452,   453,   454,   455,
     456,   457,   458,   459,   460,   425,   426,   427,   428,     0,
     656,   429,     0,   430,     0,   431,   432,   433,   434,   435,
     436,   437,   438,   439,   440,   441,   442,   443,   444,   445,
     446,   447,   448,   449,   450,   451,   452,   453,   454,   455,
     456,   457,   458,   459,   460,   425,   426,   427,   428,     0,
     667,   429,     0,   430,     0,   431,   432,   433,   434,   435,
     436,   437,   438,   439,   440,   441,   442,   443,   444,   445,
     446,   447,   448,   449,   450,   451,   452,   453,   454,   455,
     456,   457,   458,   459,   460,   425,   426,   427,   428,     0,
     688,   429,     0,   430,     0,   431,   432,   433,   434,   435,
     436,   437,   438,   439,   440,   441,   442,   443,   444,   445,
     446,   447,   448,   449,   450,   451,   452,   453,   454,   455,
     456,   457,   458,   459,   460,   425,   426,   427,   428,     0,
     703,   429,     0,   430,     0,   431,   432,   433,   434,   435,
     436,   437,   438,   439,   440,   441,   442,   443,   444,   445,
     446,   447,   448,   449,   450,   451,   452,   453,   454,   455,
     456,   457,   458,   459,   460,   425,   426,   427,   428,     0,
     710,   429,     0,   430,     0,   431,   432,   433,   434,   435,
     436,   437,   438,   439,   440,   441,   442,   443,   444,   445,
     446,   447,   448,   449,   450,   451,   452,   453,   454,   455,
     456,   457,   458,   459,   460,   425,   426,   427,   428,     0,
       0,   429,     0,   430,     0,   431,   432,   433,   434,   435,
     436,   437,   438,   439,   440,   441,   442,   443,   444,   445,
     446,   447,   448,   449,   450,   451,   452,   453,   454,   455,
     456,   457,   458,   459,   460,   426,   427,   428,     0,     0,
     429,     0,   430,     0,   431,   432,   433,   434,   435,   436,
     437,   438,   439,   440,   441,   442,   443,   444,   445,   446,
     447,   448,   449,   450,   451,   452,   453,   454,   455,   456,
     457,   458,   459,   460,   428,     0,     0,   429,     0,   430,
       0,   431,   432,   433,   434,   435,   436,   437,   438,   439,
     440,   441,   442,   443,   444,   445,   446,   447,   448,   449,
     450,   451,   452,   453,   454,   455,   456,   457,   458,   459,
     460,   429,     0,   430,     0,   431,   432,   433,   434,   435,
     436,   437,   438,   439,   440,   441,   442,   443,   444,   445,
     446,   447,   448,   449,   450,   451,   452,   453,   454,   455,
     456,   457,   458,   459,   460,   430,     0,   431,   432,   433,
     434,   435,   436,   437,   438,   439,   440,   441,   442,   443,
     444,   445,   446,   447,   448,   449,   450,   451,   452,   453,
     454,   455,   456,   457,   458,   459,   460,   496,     0,     0,
     497,     0,   498,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     499,   500,   501,   502,     0,     0,   503,   504,   431,   432,
     433,   434,   435,   436,   437,   438,   439,   440,   441,   442,
     443,   444,   445,   446,   447,   448,   449,   450,   451,   452,
     453,   454,   455,   456,   457,   458,   459,   460,   142,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     143,   144,   145,   146,     0,     0,   147,   148,   149,     0,
       0,     0,     0,   150,     0,   245,   151,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,     0,    68,    69,
      70,    71,    72,     0,    73,     0,     0,     0,     0,     0,
     374,   246,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,     0,    68,    69,    70,    71,    72,     0,    73,
       0,     0,     0,     0,     0,     0,   375,   435,   436,   437,
     438,   439,   440,   441,   442,   443,   444,   445,   446,   447,
     448,   449,   450,   451,   452,   453,   454,   455,   456,   457,
     458,   459,   460,  -271,  -271,  -271,  -271,  -271,  -271,  -271,
    -271,  -271,  -271,  -271,  -271,   447,   448,   449,   450,   451,
     452,   453,   454,   455,   456,   457,   458,   459,   460,   105,
       0,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,     0,    68,    69,    70,    71,    72,   108,    73,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,     0,
      68,    69,    70,    71,    72,   255,    73,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,     0,    68,    69,
      70,    71,    72,     0,    73
};

static const yytype_int16 yycheck[] =
{
      91,   141,   141,     0,     1,   201,   203,    98,     8,     8,
       6,    33,     8,     8,   141,    11,     8,     8,    11,    33,
      31,    79,    63,     6,   221,     8,   128,   107,    11,     8,
     159,   107,     9,   107,   125,    20,    98,   117,     1,   117,
      85,    86,   236,   237,   238,   239,   240,   241,   107,    33,
     126,   142,    71,   133,   132,    40,   185,    42,   132,    78,
     151,    80,   129,   125,   105,   107,   125,   207,   135,   196,
      63,    68,    69,    70,    93,   117,   134,   107,   128,   206,
      77,   100,   132,   105,   103,   107,   130,   131,   141,   151,
     109,   105,    68,    72,   291,    74,     9,   294,    76,   343,
     297,   298,   299,   300,   301,    68,    69,    70,    87,    88,
     107,   133,   105,   134,    77,   107,   127,    94,   132,   129,
     135,   105,   132,   107,   321,   135,   323,   104,   128,   126,
     107,   108,   127,   132,   135,   135,   132,   128,   135,   230,
     231,   232,   339,   196,   107,   342,   113,   130,   115,   107,
     347,   204,   110,   206,   207,   349,   350,   128,   352,   136,
     128,   132,   128,   126,   127,   132,   132,   128,   230,   231,
     232,   132,   135,   117,   135,   228,   420,   102,   107,   423,
     165,    94,   107,   236,   237,   238,   239,   240,   241,   174,
     115,   104,   130,   131,   107,   108,   121,    89,    30,    31,
      32,    33,   129,   330,    36,    37,   130,   131,   133,   406,
     407,   408,   409,   410,   411,   412,   413,   414,   415,   416,
     417,   418,   419,   136,   421,   422,   130,   131,   425,   426,
     427,   428,   429,   430,   431,   432,   433,   434,   435,   436,
     437,   438,   439,   440,   441,   442,   443,   444,   445,   446,
     447,   448,   449,   450,   451,   452,   453,   454,   455,   456,
     457,   458,   459,   460,   391,   461,    15,   135,    17,    18,
     467,   256,   128,   132,   107,    11,   135,   330,    27,    28,
     113,   334,   115,   113,     6,   115,     8,   128,    17,    11,
       7,   488,    73,    10,    75,    12,   349,   350,     8,   352,
     133,    11,   496,   497,   498,   499,   500,   501,   502,   503,
     504,   130,   131,    30,    31,    32,    33,   128,    11,    36,
      37,    32,    33,    36,    37,    36,    37,    63,   128,     3,
       4,     5,     6,   107,    63,     9,   110,    11,   391,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,   105,
      63,   107,    81,    82,    83,    84,   105,    11,   107,   128,
     128,   117,   128,   119,   128,   121,   122,   123,    85,    86,
     126,     8,   121,   122,   123,   107,     8,   104,   595,   135,
     597,     8,    36,   132,   105,   117,   129,   119,   107,   121,
     122,   123,   105,   130,   107,    36,    37,    38,    39,    40,
      41,    42,   616,   617,   117,     8,   119,     8,   121,   122,
     123,   107,    81,   126,    83,    84,   128,   128,   635,   129,
     637,   638,   135,   496,   497,   498,   499,   500,   501,   502,
     503,   504,   132,   650,    10,   105,    12,    48,    49,   133,
     129,   658,   129,   129,   591,    90,    91,    92,   105,   127,
     101,   105,   128,   107,    30,    31,    32,    33,   618,   618,
      36,    37,     8,   117,   117,   119,   105,   121,   122,   123,
       8,   618,   126,     8,   105,   107,     8,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   704,   120,   121,
     122,   123,   124,   105,   126,   105,   132,   105,   105,   129,
     121,   135,   130,   129,   615,   128,   107,   129,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   591,   120,
     121,   122,   123,   124,    87,   126,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,   132,   105,   107,
     134,   131,   615,   616,   617,   618,     3,     4,     5,     6,
     131,   131,     9,   131,    11,   105,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,     3,     4,     5,     6,
      18,    18,     9,   109,    11,   105,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,     3,     4,     5,     6,
     132,   129,     9,     8,    11,     8,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,   129,     3,     4,     5,
       6,   129,   128,     9,   129,    11,   133,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,     3,     4,     5,
       6,   129,   105,     9,   129,    11,   133,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,   129,   129,     3,
       4,     5,     6,   130,   131,     9,   135,    11,   129,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,   129,
       3,     4,     5,     6,   129,   131,     9,   129,    11,   129,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,   129,   131,     9,   129,    11,   129,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,   129,   131,     9,   131,    11,   129,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,     8,   105,     9,   130,    11,     8,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,     8,   105,     9,   130,    11,   105,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,   105,   105,     9,   130,    11,    11,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,    76,   105,     9,   130,    11,   105,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,   105,   105,     9,   130,    11,   128,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,   105,   105,     9,   130,    11,   133,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,   109,   105,     9,   130,    11,   105,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,   105,   130,     9,   130,    11,   105,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,   105,   130,     9,   130,    11,     8,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,     8,   131,     9,   130,    11,   128,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,   128,   131,     9,   130,    11,   129,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,   128,   105,     9,   130,    11,   130,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,   130,   130,     9,   130,    11,   128,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,   128,   130,     9,   130,    11,   135,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
       3,     4,     5,     6,   133,   130,     9,   130,    11,     8,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      71,     7,     8,     8,    10,     8,    12,   130,   105,    80,
       8,    11,   131,     8,     8,     8,     8,     8,   105,   105,
     105,   133,    93,   131,    30,    31,    32,    33,   131,   100,
      36,    37,   103,    33,    34,   133,    36,     8,   109,   133,
      30,    30,    11,    43,    44,    45,    46,   130,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,   128,   115,     7,     8,   115,    10,
     130,    12,   130,   113,   114,   115,   116,   117,    11,   119,
     120,   121,   122,   123,    17,   128,     8,     8,    31,    30,
      31,    32,    33,    31,    63,    36,    37,   115,    31,   242,
     142,   618,   511,    36,   362,   105,   463,   107,   334,    36,
     624,    -1,   112,    -1,    17,    -1,    -1,   117,   118,   119,
      -1,   121,   122,   123,    -1,   107,   126,    -1,    -1,   129,
      63,    64,    65,    66,    67,   117,   105,   119,   107,   121,
     122,   123,    -1,    11,   126,    -1,    -1,    -1,   117,    17,
     119,    -1,   121,   122,   123,    17,    -1,   126,    -1,    -1,
      63,    -1,    95,    96,    97,    -1,    99,    -1,    36,    -1,
      -1,    -1,   105,    -1,   107,    -1,    -1,    -1,    -1,   112,
      -1,    -1,    -1,    -1,   117,   118,   119,    12,   121,   122,
     123,   124,    33,   126,   127,    63,    64,    65,    66,    67,
      -1,    63,   105,    44,   107,    30,    31,    32,    33,    -1,
      -1,    36,    37,    -1,    -1,    -1,    -1,    -1,   121,   122,
     123,    -1,    -1,    -1,    -1,    -1,    -1,    95,    96,    97,
      98,    99,    -1,    -1,    -1,    -1,    -1,   105,    -1,   107,
      -1,    -1,    -1,   105,   112,   107,    -1,    -1,    -1,   117,
     118,   119,    -1,   121,   122,   123,   124,    -1,   126,   121,
     122,   123,    -1,    -1,    -1,   106,    -1,    -1,    -1,    -1,
     132,    -1,   113,   114,   115,   116,   117,    -1,   119,   120,
     121,   122,   123,     3,     4,     5,     6,    -1,   129,     9,
      -1,    11,    -1,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    -1,    -1,    -1,   113,   114,   115,   116,
      -1,    -1,   119,   120,   121,     3,     4,     5,     6,   126,
       8,     9,   129,    11,    64,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,     3,     4,     5,     6,    -1,
       8,     9,    -1,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,     3,     4,     5,     6,    -1,
       8,     9,    -1,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,     3,     4,     5,     6,    -1,
       8,     9,    -1,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,     3,     4,     5,     6,    -1,
       8,     9,    -1,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,     3,     4,     5,     6,    -1,
       8,     9,    -1,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,     3,     4,     5,     6,    -1,
       8,     9,    -1,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,     3,     4,     5,     6,    -1,
       8,     9,    -1,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,     3,     4,     5,     6,    -1,
      -1,     9,    -1,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,     4,     5,     6,    -1,    -1,
       9,    -1,    11,    -1,    13,    14,    15,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,     6,    -1,    -1,     9,    -1,    11,
      -1,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,     9,    -1,    11,    -1,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    11,    -1,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,     7,    -1,    -1,
      10,    -1,    12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      30,    31,    32,    33,    -1,    -1,    36,    37,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,   101,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     113,   114,   115,   116,    -1,    -1,   119,   120,   121,    -1,
      -1,    -1,    -1,   126,    -1,   107,   129,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,    -1,   120,   121,
     122,   123,   124,    -1,   126,    -1,    -1,    -1,    -1,    -1,
     107,   133,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,    -1,   120,   121,   122,   123,   124,    -1,   126,
      -1,    -1,    -1,    -1,    -1,    -1,   133,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,   107,
      -1,   109,   110,   111,   112,   113,   114,   115,   116,   117,
     118,    -1,   120,   121,   122,   123,   124,   107,   126,   109,
     110,   111,   112,   113,   114,   115,   116,   117,   118,    -1,
     120,   121,   122,   123,   124,   107,   126,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,    -1,   120,   121,
     122,   123,   124,    -1,   126
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    68,   138,   141,    76,     0,     1,    68,    69,    70,
      77,   107,   126,   135,   139,   140,   142,   143,   144,   145,
     146,   149,   150,   151,   152,   155,   156,   157,   158,   159,
     160,   161,   162,   165,   167,   168,   169,   128,     8,   127,
      71,    78,    80,    93,   100,   103,   109,    90,    91,    92,
     107,   107,   126,   163,   135,   135,     8,   107,   109,   110,
     111,   112,   113,   114,   115,   116,   117,   118,   120,   121,
     122,   123,   124,   126,   147,   207,     8,   107,   148,   207,
       8,    72,    74,    87,    88,    68,   127,   139,   140,   134,
       9,    94,   104,   107,   108,   136,   194,     9,    94,   107,
     108,   136,   194,    85,    86,   107,   207,   128,   107,   207,
     107,   132,   197,   117,   107,   153,   154,    89,   113,   115,
     132,   201,   201,   201,   129,   135,   128,    81,    83,    84,
     128,   128,    81,    82,    83,    84,   128,   128,   128,   128,
     181,   182,   101,   113,   114,   115,   116,   119,   120,   121,
     126,   129,   171,   172,   173,   174,   175,   192,   104,   171,
       8,     8,   105,   129,   107,   198,     8,   128,     8,   154,
     128,   113,   115,   202,   132,   199,    33,   105,   132,   195,
     117,   132,   203,   107,   206,   164,   171,   129,   129,   129,
     129,    73,    75,   105,   105,   127,    11,    17,    36,    63,
      64,    65,    66,    67,    95,    96,    97,    98,    99,   105,
     107,   112,   117,   118,   119,   121,   122,   123,   124,   126,
     183,   185,   187,   188,   189,   190,   193,   173,   128,   171,
       6,     8,    11,   132,   176,   101,    15,    17,    18,    27,
      28,   128,   176,     8,   105,   107,   133,   207,   117,    63,
     105,   107,   113,   115,   133,   107,   200,   207,     8,   105,
      33,   105,   107,   196,     8,   107,   117,   204,     8,   130,
     131,   176,   105,   105,   105,   105,   107,   126,   135,   187,
     193,   107,   125,   132,   135,    63,   105,   107,   121,   122,
     123,   132,   186,   190,   132,   186,     8,   132,    33,    34,
      43,    44,    45,    46,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,   112,
     118,   129,   184,   185,   187,   193,   107,   193,   129,   187,
      11,   105,   107,   126,   185,   191,   193,   121,   135,   128,
     135,   129,   128,   129,   184,   129,   135,   128,   132,    33,
      44,   106,   129,   170,   192,   193,   130,   171,   171,   171,
     177,    79,   134,   166,   170,   170,   170,   170,   170,   170,
     166,   130,   132,    87,   107,   133,   207,   105,    33,   105,
     107,   133,   107,   117,   133,   107,   134,   131,   131,   131,
     131,   105,    18,    18,   109,   105,   132,   184,     8,   184,
       8,   184,   184,   184,   184,   184,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   184,     3,     4,     5,     6,     9,
      11,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    64,   184,   129,   132,   135,     8,   128,   135,   128,
     105,   131,   135,   187,   135,   107,   126,   189,   193,     8,
       8,   105,   184,   105,   184,   205,   184,   205,   128,   105,
     105,   184,   105,   170,   170,   170,     7,    10,    12,    30,
      31,    32,    33,    36,    37,   102,   107,   115,   121,   133,
     179,   180,   181,    76,   105,   128,   105,   105,   105,   105,
     105,   187,   133,   109,   133,   133,   133,   184,   184,   184,
     184,   184,   184,   184,   184,   184,   184,   184,   184,   184,
     184,   205,   184,   184,   205,   130,   184,   184,   184,   184,
     184,   184,   184,   184,   184,   184,   184,   184,   184,   184,
     184,   184,   184,   184,   184,   184,   184,   184,   184,   184,
     184,   184,   184,   184,   184,   184,   184,   184,   184,   184,
     184,   184,   186,   188,   105,   105,   184,   105,   130,   107,
     110,   105,   105,     8,     8,   128,   130,   131,     8,   130,
     184,   130,   128,     8,   131,   130,   170,   170,   170,   170,
     170,   170,   170,   170,   170,   129,   128,   128,   182,    31,
     127,   178,   128,   131,   105,   130,   130,   130,   130,   135,
     133,     8,     8,     8,   130,   131,   130,   131,   131,   130,
     130,   130,   130,   130,   130,   130,   130,   130,   130,   130,
     131,   130,   130,     8,   130,   131,     8,     8,   128,     8,
       8,     8,   187,   184,     8,   184,     8,     8,   105,   192,
     193,   170,   170,   127,   178,   107,   110,    85,    86,   105,
     197,   105,   184,   184,   184,   184,   105,   184,     8,   133,
     131,   131,     8,     8,    30,    30,   133,     8,   130,   130,
     130,   130,   133,     8,   128,   115,   115,   184,   130,   130,
       8,     8,     8
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}

/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*-------------------------.
| yyparse or yypush_parse.  |
`-------------------------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{


    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 19:

/* Line 1455 of yacc.c  */
#line 175 "slghparse.y"
    { slgh->resetConstructors(); ;}
    break;

  case 20:

/* Line 1455 of yacc.c  */
#line 177 "slghparse.y"
    { slgh->setEndian(1); ;}
    break;

  case 21:

/* Line 1455 of yacc.c  */
#line 178 "slghparse.y"
    { slgh->setEndian(0); ;}
    break;

  case 22:

/* Line 1455 of yacc.c  */
#line 180 "slghparse.y"
    { slgh->setAlignment(*(yyvsp[(4) - (5)].i)); delete (yyvsp[(4) - (5)].i); ;}
    break;

  case 23:

/* Line 1455 of yacc.c  */
#line 182 "slghparse.y"
    {;}
    break;

  case 24:

/* Line 1455 of yacc.c  */
#line 184 "slghparse.y"
    { (yyval.tokensym) = slgh->defineToken((yyvsp[(3) - (6)].str),(yyvsp[(5) - (6)].i),0); ;}
    break;

  case 25:

/* Line 1455 of yacc.c  */
#line 185 "slghparse.y"
    { (yyval.tokensym) = slgh->defineToken((yyvsp[(3) - (9)].str),(yyvsp[(5) - (9)].i),-1); ;}
    break;

  case 26:

/* Line 1455 of yacc.c  */
#line 186 "slghparse.y"
    { (yyval.tokensym) = slgh->defineToken((yyvsp[(3) - (9)].str),(yyvsp[(5) - (9)].i),1); ;}
    break;

  case 27:

/* Line 1455 of yacc.c  */
#line 187 "slghparse.y"
    { (yyval.tokensym) = (yyvsp[(1) - (2)].tokensym); slgh->addTokenField((yyvsp[(1) - (2)].tokensym),(yyvsp[(2) - (2)].fieldqual)); ;}
    break;

  case 28:

/* Line 1455 of yacc.c  */
#line 188 "slghparse.y"
    { string errmsg=(yyvsp[(3) - (3)].anysym)->getName()+": redefined as a token"; yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 29:

/* Line 1455 of yacc.c  */
#line 190 "slghparse.y"
    {;}
    break;

  case 30:

/* Line 1455 of yacc.c  */
#line 192 "slghparse.y"
    { (yyval.varsym) = (yyvsp[(3) - (3)].varsym); ;}
    break;

  case 31:

/* Line 1455 of yacc.c  */
#line 193 "slghparse.y"
    { (yyval.varsym) = (yyvsp[(1) - (2)].varsym); if (!slgh->addContextField( (yyvsp[(1) - (2)].varsym), (yyvsp[(2) - (2)].fieldqual) ))
                                            { yyerror("All context definitions must come before constructors"); YYERROR; } ;}
    break;

  case 32:

/* Line 1455 of yacc.c  */
#line 196 "slghparse.y"
    { (yyval.fieldqual) = new FieldQuality((yyvsp[(1) - (7)].str),(yyvsp[(4) - (7)].i),(yyvsp[(6) - (7)].i)); ;}
    break;

  case 33:

/* Line 1455 of yacc.c  */
#line 197 "slghparse.y"
    { delete (yyvsp[(4) - (7)].i); delete (yyvsp[(6) - (7)].i); string errmsg = (yyvsp[(1) - (7)].anysym)->getName()+": redefined as field"; yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 34:

/* Line 1455 of yacc.c  */
#line 198 "slghparse.y"
    { (yyval.fieldqual) = (yyvsp[(1) - (2)].fieldqual); (yyval.fieldqual)->signext = true; ;}
    break;

  case 35:

/* Line 1455 of yacc.c  */
#line 199 "slghparse.y"
    { (yyval.fieldqual) = (yyvsp[(1) - (2)].fieldqual); (yyval.fieldqual)->hex = true; ;}
    break;

  case 36:

/* Line 1455 of yacc.c  */
#line 200 "slghparse.y"
    { (yyval.fieldqual) = (yyvsp[(1) - (2)].fieldqual); (yyval.fieldqual)->hex = false; ;}
    break;

  case 37:

/* Line 1455 of yacc.c  */
#line 202 "slghparse.y"
    { (yyval.fieldqual) = new FieldQuality((yyvsp[(1) - (7)].str),(yyvsp[(4) - (7)].i),(yyvsp[(6) - (7)].i)); ;}
    break;

  case 38:

/* Line 1455 of yacc.c  */
#line 203 "slghparse.y"
    { delete (yyvsp[(4) - (7)].i); delete (yyvsp[(6) - (7)].i); string errmsg = (yyvsp[(1) - (7)].anysym)->getName()+": redefined as field"; yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 39:

/* Line 1455 of yacc.c  */
#line 204 "slghparse.y"
    { (yyval.fieldqual) = (yyvsp[(1) - (2)].fieldqual); (yyval.fieldqual)->signext = true; ;}
    break;

  case 40:

/* Line 1455 of yacc.c  */
#line 205 "slghparse.y"
    { (yyval.fieldqual) = (yyvsp[(1) - (2)].fieldqual); (yyval.fieldqual)->flow = false; ;}
    break;

  case 41:

/* Line 1455 of yacc.c  */
#line 206 "slghparse.y"
    { (yyval.fieldqual) = (yyvsp[(1) - (2)].fieldqual); (yyval.fieldqual)->hex = true; ;}
    break;

  case 42:

/* Line 1455 of yacc.c  */
#line 207 "slghparse.y"
    { (yyval.fieldqual) = (yyvsp[(1) - (2)].fieldqual); (yyval.fieldqual)->hex = false; ;}
    break;

  case 43:

/* Line 1455 of yacc.c  */
#line 209 "slghparse.y"
    { slgh->newSpace((yyvsp[(1) - (2)].spacequal)); ;}
    break;

  case 44:

/* Line 1455 of yacc.c  */
#line 211 "slghparse.y"
    { (yyval.spacequal) = new SpaceQuality(*(yyvsp[(3) - (3)].str)); delete (yyvsp[(3) - (3)].str); ;}
    break;

  case 45:

/* Line 1455 of yacc.c  */
#line 212 "slghparse.y"
    { string errmsg = (yyvsp[(3) - (3)].anysym)->getName()+": redefined as space"; yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 46:

/* Line 1455 of yacc.c  */
#line 213 "slghparse.y"
    { (yyval.spacequal) = (yyvsp[(1) - (4)].spacequal); (yyval.spacequal)->type = SpaceQuality::ramtype; ;}
    break;

  case 47:

/* Line 1455 of yacc.c  */
#line 214 "slghparse.y"
    { (yyval.spacequal) = (yyvsp[(1) - (4)].spacequal); (yyval.spacequal)->type = SpaceQuality::registertype; ;}
    break;

  case 48:

/* Line 1455 of yacc.c  */
#line 215 "slghparse.y"
    { (yyval.spacequal) = (yyvsp[(1) - (4)].spacequal); (yyval.spacequal)->size = *(yyvsp[(4) - (4)].i); delete (yyvsp[(4) - (4)].i); ;}
    break;

  case 49:

/* Line 1455 of yacc.c  */
#line 216 "slghparse.y"
    { (yyval.spacequal) = (yyvsp[(1) - (4)].spacequal); (yyval.spacequal)->wordsize = *(yyvsp[(4) - (4)].i); delete (yyvsp[(4) - (4)].i); ;}
    break;

  case 50:

/* Line 1455 of yacc.c  */
#line 217 "slghparse.y"
    { (yyval.spacequal) = (yyvsp[(1) - (2)].spacequal); (yyval.spacequal)->isdefault = true; ;}
    break;

  case 51:

/* Line 1455 of yacc.c  */
#line 219 "slghparse.y"
    {
               slgh->defineVarnodes((yyvsp[(2) - (10)].spacesym),(yyvsp[(5) - (10)].i),(yyvsp[(8) - (10)].i),(yyvsp[(9) - (10)].strlist)); ;}
    break;

  case 52:

/* Line 1455 of yacc.c  */
#line 221 "slghparse.y"
    { yyerror("Parsed integer is too big (overflow)"); YYERROR; ;}
    break;

  case 56:

/* Line 1455 of yacc.c  */
#line 228 "slghparse.y"
    {
               slgh->defineBitrange((yyvsp[(1) - (8)].str),(yyvsp[(3) - (8)].varsym),(uint4)*(yyvsp[(5) - (8)].i),(uint4)*(yyvsp[(7) - (8)].i)); delete (yyvsp[(5) - (8)].i); delete (yyvsp[(7) - (8)].i); ;}
    break;

  case 57:

/* Line 1455 of yacc.c  */
#line 231 "slghparse.y"
    { slgh->addUserOp((yyvsp[(3) - (4)].strlist)); ;}
    break;

  case 58:

/* Line 1455 of yacc.c  */
#line 233 "slghparse.y"
    { slgh->attachValues((yyvsp[(3) - (5)].symlist),(yyvsp[(4) - (5)].biglist)); ;}
    break;

  case 59:

/* Line 1455 of yacc.c  */
#line 235 "slghparse.y"
    { slgh->attachNames((yyvsp[(3) - (5)].symlist),(yyvsp[(4) - (5)].strlist)); ;}
    break;

  case 60:

/* Line 1455 of yacc.c  */
#line 237 "slghparse.y"
    { slgh->attachVarnodes((yyvsp[(3) - (5)].symlist),(yyvsp[(4) - (5)].symlist)); ;}
    break;

  case 61:

/* Line 1455 of yacc.c  */
#line 239 "slghparse.y"
    { slgh->buildMacro((yyvsp[(1) - (4)].macrosym),(yyvsp[(3) - (4)].sem)); ;}
    break;

  case 62:

/* Line 1455 of yacc.c  */
#line 242 "slghparse.y"
    {  slgh->pushWith((yyvsp[(2) - (6)].subtablesym),(yyvsp[(4) - (6)].pateq),(yyvsp[(5) - (6)].contop)); ;}
    break;

  case 66:

/* Line 1455 of yacc.c  */
#line 248 "slghparse.y"
    { slgh->popWith(); ;}
    break;

  case 67:

/* Line 1455 of yacc.c  */
#line 250 "slghparse.y"
    { (yyval.subtablesym) = (SubtableSymbol *)0; ;}
    break;

  case 68:

/* Line 1455 of yacc.c  */
#line 251 "slghparse.y"
    { (yyval.subtablesym) = (yyvsp[(1) - (1)].subtablesym); ;}
    break;

  case 69:

/* Line 1455 of yacc.c  */
#line 252 "slghparse.y"
    { (yyval.subtablesym) = slgh->newTable((yyvsp[(1) - (1)].str)); ;}
    break;

  case 70:

/* Line 1455 of yacc.c  */
#line 255 "slghparse.y"
    { (yyval.pateq) = (PatternEquation *)0; ;}
    break;

  case 71:

/* Line 1455 of yacc.c  */
#line 256 "slghparse.y"
    { (yyval.pateq) = (yyvsp[(1) - (1)].pateq); ;}
    break;

  case 72:

/* Line 1455 of yacc.c  */
#line 259 "slghparse.y"
    { (yyval.macrosym) = slgh->createMacro((yyvsp[(2) - (5)].str),(yyvsp[(4) - (5)].strlist)); ;}
    break;

  case 73:

/* Line 1455 of yacc.c  */
#line 261 "slghparse.y"
    { (yyval.sectionstart) = slgh->standaloneSection((yyvsp[(2) - (3)].sem)); ;}
    break;

  case 74:

/* Line 1455 of yacc.c  */
#line 262 "slghparse.y"
    { (yyval.sectionstart) = slgh->finalNamedSection((yyvsp[(2) - (4)].sectionstart),(yyvsp[(3) - (4)].sem)); ;}
    break;

  case 75:

/* Line 1455 of yacc.c  */
#line 263 "slghparse.y"
    { (yyval.sectionstart) = (SectionVector *)0; ;}
    break;

  case 76:

/* Line 1455 of yacc.c  */
#line 265 "slghparse.y"
    { slgh->buildConstructor((yyvsp[(1) - (5)].construct),(yyvsp[(3) - (5)].pateq),(yyvsp[(4) - (5)].contop),(yyvsp[(5) - (5)].sectionstart)); ;}
    break;

  case 77:

/* Line 1455 of yacc.c  */
#line 266 "slghparse.y"
    { slgh->buildConstructor((yyvsp[(1) - (5)].construct),(yyvsp[(3) - (5)].pateq),(yyvsp[(4) - (5)].contop),(yyvsp[(5) - (5)].sectionstart)); ;}
    break;

  case 78:

/* Line 1455 of yacc.c  */
#line 268 "slghparse.y"
    { (yyval.construct) = (yyvsp[(1) - (2)].construct); (yyval.construct)->addSyntax(*(yyvsp[(2) - (2)].str)); delete (yyvsp[(2) - (2)].str); ;}
    break;

  case 79:

/* Line 1455 of yacc.c  */
#line 269 "slghparse.y"
    { (yyval.construct) = (yyvsp[(1) - (2)].construct); (yyval.construct)->addSyntax(*(yyvsp[(2) - (2)].str)); delete (yyvsp[(2) - (2)].str); ;}
    break;

  case 80:

/* Line 1455 of yacc.c  */
#line 270 "slghparse.y"
    { (yyval.construct) = (yyvsp[(1) - (2)].construct); if (slgh->isInRoot((yyvsp[(1) - (2)].construct))) { (yyval.construct)->addSyntax(*(yyvsp[(2) - (2)].str)); delete (yyvsp[(2) - (2)].str); } else slgh->newOperand((yyvsp[(1) - (2)].construct),(yyvsp[(2) - (2)].str)); ;}
    break;

  case 81:

/* Line 1455 of yacc.c  */
#line 271 "slghparse.y"
    { (yyval.construct) = (yyvsp[(1) - (2)].construct); if (!slgh->isInRoot((yyvsp[(1) - (2)].construct))) { yyerror("Unexpected '^' at start of print pieces");  YYERROR; } ;}
    break;

  case 82:

/* Line 1455 of yacc.c  */
#line 272 "slghparse.y"
    { (yyval.construct) = (yyvsp[(1) - (2)].construct); ;}
    break;

  case 83:

/* Line 1455 of yacc.c  */
#line 273 "slghparse.y"
    { (yyval.construct) = (yyvsp[(1) - (2)].construct); (yyval.construct)->addSyntax(*(yyvsp[(2) - (2)].str)); delete (yyvsp[(2) - (2)].str); ;}
    break;

  case 84:

/* Line 1455 of yacc.c  */
#line 274 "slghparse.y"
    { (yyval.construct) = (yyvsp[(1) - (2)].construct); (yyval.construct)->addSyntax(*(yyvsp[(2) - (2)].str)); delete (yyvsp[(2) - (2)].str); ;}
    break;

  case 85:

/* Line 1455 of yacc.c  */
#line 275 "slghparse.y"
    { (yyval.construct) = (yyvsp[(1) - (2)].construct); (yyval.construct)->addSyntax(string(" ")); ;}
    break;

  case 86:

/* Line 1455 of yacc.c  */
#line 276 "slghparse.y"
    { (yyval.construct) = (yyvsp[(1) - (2)].construct); slgh->newOperand((yyvsp[(1) - (2)].construct),(yyvsp[(2) - (2)].str)); ;}
    break;

  case 87:

/* Line 1455 of yacc.c  */
#line 278 "slghparse.y"
    { (yyval.construct) = slgh->createConstructor((yyvsp[(1) - (2)].subtablesym)); ;}
    break;

  case 88:

/* Line 1455 of yacc.c  */
#line 279 "slghparse.y"
    { SubtableSymbol *sym=slgh->newTable((yyvsp[(1) - (2)].str)); (yyval.construct) = slgh->createConstructor(sym); ;}
    break;

  case 89:

/* Line 1455 of yacc.c  */
#line 280 "slghparse.y"
    { (yyval.construct) = slgh->createConstructor((SubtableSymbol *)0); ;}
    break;

  case 90:

/* Line 1455 of yacc.c  */
#line 281 "slghparse.y"
    { (yyval.construct) = (yyvsp[(1) - (2)].construct); ;}
    break;

  case 91:

/* Line 1455 of yacc.c  */
#line 283 "slghparse.y"
    { (yyval.patexp) = new ConstantValue(*(yyvsp[(1) - (1)].big)); delete (yyvsp[(1) - (1)].big); ;}
    break;

  case 92:

/* Line 1455 of yacc.c  */
#line 285 "slghparse.y"
    { if ((actionon==1)&&((yyvsp[(1) - (1)].famsym)->getType() != SleighSymbol::context_symbol))
                                             { string errmsg="Global symbol "+(yyvsp[(1) - (1)].famsym)->getName(); errmsg += " is not allowed in action expression"; yyerror(errmsg.c_str()); } (yyval.patexp) = (yyvsp[(1) - (1)].famsym)->getPatternValue(); ;}
    break;

  case 93:

/* Line 1455 of yacc.c  */
#line 288 "slghparse.y"
    { (yyval.patexp) = (yyvsp[(1) - (1)].specsym)->getPatternExpression(); ;}
    break;

  case 94:

/* Line 1455 of yacc.c  */
#line 289 "slghparse.y"
    { (yyval.patexp) = (yyvsp[(2) - (3)].patexp); ;}
    break;

  case 95:

/* Line 1455 of yacc.c  */
#line 290 "slghparse.y"
    { (yyval.patexp) = new PlusExpression((yyvsp[(1) - (3)].patexp),(yyvsp[(3) - (3)].patexp)); ;}
    break;

  case 96:

/* Line 1455 of yacc.c  */
#line 291 "slghparse.y"
    { (yyval.patexp) = new SubExpression((yyvsp[(1) - (3)].patexp),(yyvsp[(3) - (3)].patexp)); ;}
    break;

  case 97:

/* Line 1455 of yacc.c  */
#line 292 "slghparse.y"
    { (yyval.patexp) = new MultExpression((yyvsp[(1) - (3)].patexp),(yyvsp[(3) - (3)].patexp)); ;}
    break;

  case 98:

/* Line 1455 of yacc.c  */
#line 293 "slghparse.y"
    { (yyval.patexp) = new LeftShiftExpression((yyvsp[(1) - (3)].patexp),(yyvsp[(3) - (3)].patexp)); ;}
    break;

  case 99:

/* Line 1455 of yacc.c  */
#line 294 "slghparse.y"
    { (yyval.patexp) = new RightShiftExpression((yyvsp[(1) - (3)].patexp),(yyvsp[(3) - (3)].patexp)); ;}
    break;

  case 100:

/* Line 1455 of yacc.c  */
#line 295 "slghparse.y"
    { (yyval.patexp) = new AndExpression((yyvsp[(1) - (3)].patexp),(yyvsp[(3) - (3)].patexp)); ;}
    break;

  case 101:

/* Line 1455 of yacc.c  */
#line 296 "slghparse.y"
    { (yyval.patexp) = new OrExpression((yyvsp[(1) - (3)].patexp),(yyvsp[(3) - (3)].patexp)); ;}
    break;

  case 102:

/* Line 1455 of yacc.c  */
#line 297 "slghparse.y"
    { (yyval.patexp) = new XorExpression((yyvsp[(1) - (3)].patexp),(yyvsp[(3) - (3)].patexp)); ;}
    break;

  case 103:

/* Line 1455 of yacc.c  */
#line 298 "slghparse.y"
    { (yyval.patexp) = new DivExpression((yyvsp[(1) - (3)].patexp),(yyvsp[(3) - (3)].patexp)); ;}
    break;

  case 104:

/* Line 1455 of yacc.c  */
#line 299 "slghparse.y"
    { (yyval.patexp) = new MinusExpression((yyvsp[(2) - (2)].patexp)); ;}
    break;

  case 105:

/* Line 1455 of yacc.c  */
#line 300 "slghparse.y"
    { (yyval.patexp) = new NotExpression((yyvsp[(2) - (2)].patexp)); ;}
    break;

  case 107:

/* Line 1455 of yacc.c  */
#line 303 "slghparse.y"
    { (yyval.pateq) = new EquationAnd((yyvsp[(1) - (3)].pateq),(yyvsp[(3) - (3)].pateq)); ;}
    break;

  case 108:

/* Line 1455 of yacc.c  */
#line 304 "slghparse.y"
    { (yyval.pateq) = new EquationOr((yyvsp[(1) - (3)].pateq),(yyvsp[(3) - (3)].pateq)); ;}
    break;

  case 109:

/* Line 1455 of yacc.c  */
#line 305 "slghparse.y"
    { (yyval.pateq) = new EquationCat((yyvsp[(1) - (3)].pateq),(yyvsp[(3) - (3)].pateq)); ;}
    break;

  case 110:

/* Line 1455 of yacc.c  */
#line 307 "slghparse.y"
    { (yyval.pateq) = new EquationLeftEllipsis((yyvsp[(2) - (2)].pateq)); ;}
    break;

  case 112:

/* Line 1455 of yacc.c  */
#line 310 "slghparse.y"
    { (yyval.pateq) = new EquationRightEllipsis((yyvsp[(1) - (2)].pateq)); ;}
    break;

  case 115:

/* Line 1455 of yacc.c  */
#line 314 "slghparse.y"
    { (yyval.pateq) = (yyvsp[(2) - (3)].pateq); ;}
    break;

  case 116:

/* Line 1455 of yacc.c  */
#line 316 "slghparse.y"
    { (yyval.pateq) = new EqualEquation((yyvsp[(1) - (3)].famsym)->getPatternValue(),(yyvsp[(3) - (3)].patexp)); ;}
    break;

  case 117:

/* Line 1455 of yacc.c  */
#line 317 "slghparse.y"
    { (yyval.pateq) = new NotEqualEquation((yyvsp[(1) - (3)].famsym)->getPatternValue(),(yyvsp[(3) - (3)].patexp)); ;}
    break;

  case 118:

/* Line 1455 of yacc.c  */
#line 318 "slghparse.y"
    { (yyval.pateq) = new LessEquation((yyvsp[(1) - (3)].famsym)->getPatternValue(),(yyvsp[(3) - (3)].patexp)); ;}
    break;

  case 119:

/* Line 1455 of yacc.c  */
#line 319 "slghparse.y"
    { (yyval.pateq) = new LessEqualEquation((yyvsp[(1) - (3)].famsym)->getPatternValue(),(yyvsp[(3) - (3)].patexp)); ;}
    break;

  case 120:

/* Line 1455 of yacc.c  */
#line 320 "slghparse.y"
    { (yyval.pateq) = new GreaterEquation((yyvsp[(1) - (3)].famsym)->getPatternValue(),(yyvsp[(3) - (3)].patexp)); ;}
    break;

  case 121:

/* Line 1455 of yacc.c  */
#line 321 "slghparse.y"
    { (yyval.pateq) = new GreaterEqualEquation((yyvsp[(1) - (3)].famsym)->getPatternValue(),(yyvsp[(3) - (3)].patexp)); ;}
    break;

  case 122:

/* Line 1455 of yacc.c  */
#line 322 "slghparse.y"
    { (yyval.pateq) = slgh->constrainOperand((yyvsp[(1) - (3)].operandsym),(yyvsp[(3) - (3)].patexp)); 
                                          if ((yyval.pateq) == (PatternEquation *)0) 
                                            { string errmsg="Constraining currently undefined operand "+(yyvsp[(1) - (3)].operandsym)->getName(); yyerror(errmsg.c_str()); } ;}
    break;

  case 123:

/* Line 1455 of yacc.c  */
#line 325 "slghparse.y"
    { (yyval.pateq) = new OperandEquation((yyvsp[(1) - (1)].operandsym)->getIndex()); slgh->selfDefine((yyvsp[(1) - (1)].operandsym)); ;}
    break;

  case 124:

/* Line 1455 of yacc.c  */
#line 326 "slghparse.y"
    { (yyval.pateq) = new UnconstrainedEquation((yyvsp[(1) - (1)].specsym)->getPatternExpression()); ;}
    break;

  case 125:

/* Line 1455 of yacc.c  */
#line 327 "slghparse.y"
    { (yyval.pateq) = slgh->defineInvisibleOperand((yyvsp[(1) - (1)].famsym)); ;}
    break;

  case 126:

/* Line 1455 of yacc.c  */
#line 328 "slghparse.y"
    { (yyval.pateq) = slgh->defineInvisibleOperand((yyvsp[(1) - (1)].subtablesym)); ;}
    break;

  case 127:

/* Line 1455 of yacc.c  */
#line 330 "slghparse.y"
    { (yyval.contop) = (vector<ContextChange *> *)0; ;}
    break;

  case 128:

/* Line 1455 of yacc.c  */
#line 331 "slghparse.y"
    { (yyval.contop) = (yyvsp[(2) - (3)].contop); ;}
    break;

  case 129:

/* Line 1455 of yacc.c  */
#line 333 "slghparse.y"
    { (yyval.contop) = new vector<ContextChange *>; ;}
    break;

  case 130:

/* Line 1455 of yacc.c  */
#line 334 "slghparse.y"
    { (yyval.contop) = (yyvsp[(1) - (5)].contop); if (!slgh->contextMod((yyvsp[(1) - (5)].contop),(yyvsp[(2) - (5)].contextsym),(yyvsp[(4) - (5)].patexp))) { string errmsg="Cannot use 'inst_next' to set context variable: "+(yyvsp[(2) - (5)].contextsym)->getName(); yyerror(errmsg.c_str()); YYERROR; } ;}
    break;

  case 131:

/* Line 1455 of yacc.c  */
#line 335 "slghparse.y"
    { (yyval.contop) = (yyvsp[(1) - (8)].contop); slgh->contextSet((yyvsp[(1) - (8)].contop),(yyvsp[(4) - (8)].famsym),(yyvsp[(6) - (8)].contextsym)); ;}
    break;

  case 132:

/* Line 1455 of yacc.c  */
#line 336 "slghparse.y"
    { (yyval.contop) = (yyvsp[(1) - (8)].contop); slgh->contextSet((yyvsp[(1) - (8)].contop),(yyvsp[(4) - (8)].specsym),(yyvsp[(6) - (8)].contextsym)); ;}
    break;

  case 133:

/* Line 1455 of yacc.c  */
#line 337 "slghparse.y"
    { (yyval.contop) = (yyvsp[(1) - (5)].contop); slgh->defineOperand((yyvsp[(2) - (5)].operandsym),(yyvsp[(4) - (5)].patexp)); ;}
    break;

  case 134:

/* Line 1455 of yacc.c  */
#line 338 "slghparse.y"
    { string errmsg="Expecting context symbol, not "+*(yyvsp[(2) - (2)].str); delete (yyvsp[(2) - (2)].str); yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 135:

/* Line 1455 of yacc.c  */
#line 340 "slghparse.y"
    { (yyval.sectionsym) = slgh->newSectionSymbol( *(yyvsp[(2) - (3)].str) ); delete (yyvsp[(2) - (3)].str); ;}
    break;

  case 136:

/* Line 1455 of yacc.c  */
#line 341 "slghparse.y"
    { (yyval.sectionsym) = (yyvsp[(2) - (3)].sectionsym); ;}
    break;

  case 137:

/* Line 1455 of yacc.c  */
#line 343 "slghparse.y"
    { (yyval.sectionstart) = slgh->firstNamedSection((yyvsp[(1) - (2)].sem),(yyvsp[(2) - (2)].sectionsym)); ;}
    break;

  case 138:

/* Line 1455 of yacc.c  */
#line 345 "slghparse.y"
    { (yyval.sectionstart) = (yyvsp[(1) - (1)].sectionstart); ;}
    break;

  case 139:

/* Line 1455 of yacc.c  */
#line 346 "slghparse.y"
    { (yyval.sectionstart) = slgh->nextNamedSection((yyvsp[(1) - (3)].sectionstart),(yyvsp[(2) - (3)].sem),(yyvsp[(3) - (3)].sectionsym)); ;}
    break;

  case 140:

/* Line 1455 of yacc.c  */
#line 348 "slghparse.y"
    { (yyval.sem) = (yyvsp[(1) - (1)].sem); if ((yyval.sem)->getOpvec().empty() && ((yyval.sem)->getResult() == (HandleTpl *)0)) slgh->recordNop(); ;}
    break;

  case 141:

/* Line 1455 of yacc.c  */
#line 349 "slghparse.y"
    { (yyval.sem) = slgh->setResultVarnode((yyvsp[(1) - (4)].sem),(yyvsp[(3) - (4)].varnode)); ;}
    break;

  case 142:

/* Line 1455 of yacc.c  */
#line 350 "slghparse.y"
    { (yyval.sem) = slgh->setResultStarVarnode((yyvsp[(1) - (5)].sem),(yyvsp[(3) - (5)].starqual),(yyvsp[(4) - (5)].varnode)); ;}
    break;

  case 143:

/* Line 1455 of yacc.c  */
#line 351 "slghparse.y"
    { string errmsg="Unknown export varnode: "+*(yyvsp[(3) - (3)].str); delete (yyvsp[(3) - (3)].str); yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 144:

/* Line 1455 of yacc.c  */
#line 352 "slghparse.y"
    { string errmsg="Unknown pointer varnode: "+*(yyvsp[(4) - (4)].str); delete (yyvsp[(3) - (4)].starqual); delete (yyvsp[(4) - (4)].str); yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 145:

/* Line 1455 of yacc.c  */
#line 354 "slghparse.y"
    { (yyval.sem) = new ConstructTpl(); ;}
    break;

  case 146:

/* Line 1455 of yacc.c  */
#line 355 "slghparse.y"
    { (yyval.sem) = (yyvsp[(1) - (2)].sem); if (!(yyval.sem)->addOpList(*(yyvsp[(2) - (2)].stmt))) { delete (yyvsp[(2) - (2)].stmt); yyerror("Multiple delayslot declarations"); YYERROR; } delete (yyvsp[(2) - (2)].stmt); ;}
    break;

  case 147:

/* Line 1455 of yacc.c  */
#line 356 "slghparse.y"
    { (yyval.sem) = (yyvsp[(1) - (4)].sem); slgh->pcode.newLocalDefinition((yyvsp[(3) - (4)].str)); ;}
    break;

  case 148:

/* Line 1455 of yacc.c  */
#line 357 "slghparse.y"
    { (yyval.sem) = (yyvsp[(1) - (6)].sem); slgh->pcode.newLocalDefinition((yyvsp[(3) - (6)].str),*(yyvsp[(5) - (6)].i)); delete (yyvsp[(5) - (6)].i); ;}
    break;

  case 149:

/* Line 1455 of yacc.c  */
#line 359 "slghparse.y"
    { (yyvsp[(3) - (4)].tree)->setOutput((yyvsp[(1) - (4)].varnode)); (yyval.stmt) = ExprTree::toVector((yyvsp[(3) - (4)].tree)); ;}
    break;

  case 150:

/* Line 1455 of yacc.c  */
#line 360 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.newOutput(true,(yyvsp[(4) - (5)].tree),(yyvsp[(2) - (5)].str)); ;}
    break;

  case 151:

/* Line 1455 of yacc.c  */
#line 361 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.newOutput(false,(yyvsp[(3) - (4)].tree),(yyvsp[(1) - (4)].str)); ;}
    break;

  case 152:

/* Line 1455 of yacc.c  */
#line 362 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.newOutput(true,(yyvsp[(6) - (7)].tree),(yyvsp[(2) - (7)].str),*(yyvsp[(4) - (7)].i)); delete (yyvsp[(4) - (7)].i); ;}
    break;

  case 153:

/* Line 1455 of yacc.c  */
#line 363 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.newOutput(true,(yyvsp[(5) - (6)].tree),(yyvsp[(1) - (6)].str),*(yyvsp[(3) - (6)].i)); delete (yyvsp[(3) - (6)].i); ;}
    break;

  case 154:

/* Line 1455 of yacc.c  */
#line 364 "slghparse.y"
    { (yyval.stmt) = (vector<OpTpl *> *)0; string errmsg = "Redefinition of symbol: "+(yyvsp[(2) - (3)].specsym)->getName(); yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 155:

/* Line 1455 of yacc.c  */
#line 365 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.createStore((yyvsp[(1) - (5)].starqual),(yyvsp[(2) - (5)].tree),(yyvsp[(4) - (5)].tree)); ;}
    break;

  case 156:

/* Line 1455 of yacc.c  */
#line 366 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.createUserOpNoOut((yyvsp[(1) - (5)].useropsym),(yyvsp[(3) - (5)].param)); ;}
    break;

  case 157:

/* Line 1455 of yacc.c  */
#line 367 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.assignBitRange((yyvsp[(1) - (9)].varnode),(uint4)*(yyvsp[(3) - (9)].i),(uint4)*(yyvsp[(5) - (9)].i),(yyvsp[(8) - (9)].tree)); delete (yyvsp[(3) - (9)].i), delete (yyvsp[(5) - (9)].i); ;}
    break;

  case 158:

/* Line 1455 of yacc.c  */
#line 368 "slghparse.y"
    { (yyval.stmt)=slgh->pcode.assignBitRange((yyvsp[(1) - (4)].bitsym)->getParentSymbol()->getVarnode(),(yyvsp[(1) - (4)].bitsym)->getBitOffset(),(yyvsp[(1) - (4)].bitsym)->numBits(),(yyvsp[(3) - (4)].tree)); ;}
    break;

  case 159:

/* Line 1455 of yacc.c  */
#line 369 "slghparse.y"
    { delete (yyvsp[(1) - (4)].varnode); delete (yyvsp[(3) - (4)].i); yyerror("Illegal truncation on left-hand side of assignment"); YYERROR; ;}
    break;

  case 160:

/* Line 1455 of yacc.c  */
#line 370 "slghparse.y"
    { delete (yyvsp[(1) - (4)].varnode); delete (yyvsp[(3) - (4)].i); yyerror("Illegal subpiece on left-hand side of assignment"); YYERROR; ;}
    break;

  case 161:

/* Line 1455 of yacc.c  */
#line 371 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.createOpConst(BUILD,(yyvsp[(2) - (3)].operandsym)->getIndex()); ;}
    break;

  case 162:

/* Line 1455 of yacc.c  */
#line 372 "slghparse.y"
    { (yyval.stmt) = slgh->createCrossBuild((yyvsp[(2) - (5)].varnode),(yyvsp[(4) - (5)].sectionsym)); ;}
    break;

  case 163:

/* Line 1455 of yacc.c  */
#line 373 "slghparse.y"
    { (yyval.stmt) = slgh->createCrossBuild((yyvsp[(2) - (5)].varnode),slgh->newSectionSymbol(*(yyvsp[(4) - (5)].str))); delete (yyvsp[(4) - (5)].str); ;}
    break;

  case 164:

/* Line 1455 of yacc.c  */
#line 374 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.createOpConst(DELAY_SLOT,*(yyvsp[(3) - (5)].i)); delete (yyvsp[(3) - (5)].i); ;}
    break;

  case 165:

/* Line 1455 of yacc.c  */
#line 375 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.createOpNoOut(CPUI_BRANCH,new ExprTree((yyvsp[(2) - (3)].varnode))); ;}
    break;

  case 166:

/* Line 1455 of yacc.c  */
#line 376 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.createOpNoOut(CPUI_CBRANCH,new ExprTree((yyvsp[(4) - (5)].varnode)),(yyvsp[(2) - (5)].tree)); ;}
    break;

  case 167:

/* Line 1455 of yacc.c  */
#line 377 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.createOpNoOut(CPUI_BRANCHIND,(yyvsp[(3) - (5)].tree)); ;}
    break;

  case 168:

/* Line 1455 of yacc.c  */
#line 378 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.createOpNoOut(CPUI_CALL,new ExprTree((yyvsp[(2) - (3)].varnode))); ;}
    break;

  case 169:

/* Line 1455 of yacc.c  */
#line 379 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.createOpNoOut(CPUI_CALLIND,(yyvsp[(3) - (5)].tree)); ;}
    break;

  case 170:

/* Line 1455 of yacc.c  */
#line 380 "slghparse.y"
    { yyerror("Must specify an indirect parameter for return"); YYERROR; ;}
    break;

  case 171:

/* Line 1455 of yacc.c  */
#line 381 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.createOpNoOut(CPUI_RETURN,(yyvsp[(3) - (5)].tree)); ;}
    break;

  case 172:

/* Line 1455 of yacc.c  */
#line 382 "slghparse.y"
    { (yyval.stmt) = slgh->createMacroUse((yyvsp[(1) - (5)].macrosym),(yyvsp[(3) - (5)].param)); ;}
    break;

  case 173:

/* Line 1455 of yacc.c  */
#line 383 "slghparse.y"
    { (yyval.stmt) = slgh->pcode.placeLabel( (yyvsp[(1) - (1)].labelsym) ); ;}
    break;

  case 174:

/* Line 1455 of yacc.c  */
#line 385 "slghparse.y"
    { (yyval.tree) = new ExprTree((yyvsp[(1) - (1)].varnode)); ;}
    break;

  case 175:

/* Line 1455 of yacc.c  */
#line 386 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createLoad((yyvsp[(1) - (2)].starqual),(yyvsp[(2) - (2)].tree)); ;}
    break;

  case 176:

/* Line 1455 of yacc.c  */
#line 387 "slghparse.y"
    { (yyval.tree) = (yyvsp[(2) - (3)].tree); ;}
    break;

  case 177:

/* Line 1455 of yacc.c  */
#line 388 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_ADD,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 178:

/* Line 1455 of yacc.c  */
#line 389 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_SUB,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 179:

/* Line 1455 of yacc.c  */
#line 390 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_EQUAL,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 180:

/* Line 1455 of yacc.c  */
#line 391 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_NOTEQUAL,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 181:

/* Line 1455 of yacc.c  */
#line 392 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_LESS,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 182:

/* Line 1455 of yacc.c  */
#line 393 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_LESSEQUAL,(yyvsp[(3) - (3)].tree),(yyvsp[(1) - (3)].tree)); ;}
    break;

  case 183:

/* Line 1455 of yacc.c  */
#line 394 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_LESSEQUAL,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 184:

/* Line 1455 of yacc.c  */
#line 395 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_LESS,(yyvsp[(3) - (3)].tree),(yyvsp[(1) - (3)].tree)); ;}
    break;

  case 185:

/* Line 1455 of yacc.c  */
#line 396 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_SLESS,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 186:

/* Line 1455 of yacc.c  */
#line 397 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_SLESSEQUAL,(yyvsp[(3) - (3)].tree),(yyvsp[(1) - (3)].tree)); ;}
    break;

  case 187:

/* Line 1455 of yacc.c  */
#line 398 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_SLESSEQUAL,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 188:

/* Line 1455 of yacc.c  */
#line 399 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_SLESS,(yyvsp[(3) - (3)].tree),(yyvsp[(1) - (3)].tree)); ;}
    break;

  case 189:

/* Line 1455 of yacc.c  */
#line 400 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_2COMP,(yyvsp[(2) - (2)].tree)); ;}
    break;

  case 190:

/* Line 1455 of yacc.c  */
#line 401 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_NEGATE,(yyvsp[(2) - (2)].tree)); ;}
    break;

  case 191:

/* Line 1455 of yacc.c  */
#line 402 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_XOR,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 192:

/* Line 1455 of yacc.c  */
#line 403 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_AND,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 193:

/* Line 1455 of yacc.c  */
#line 404 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_OR,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 194:

/* Line 1455 of yacc.c  */
#line 405 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_LEFT,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 195:

/* Line 1455 of yacc.c  */
#line 406 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_RIGHT,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 196:

/* Line 1455 of yacc.c  */
#line 407 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_SRIGHT,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 197:

/* Line 1455 of yacc.c  */
#line 408 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_MULT,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 198:

/* Line 1455 of yacc.c  */
#line 409 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_DIV,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 199:

/* Line 1455 of yacc.c  */
#line 410 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_SDIV,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 200:

/* Line 1455 of yacc.c  */
#line 411 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_REM,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 201:

/* Line 1455 of yacc.c  */
#line 412 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_SREM,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 202:

/* Line 1455 of yacc.c  */
#line 413 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_BOOL_NEGATE,(yyvsp[(2) - (2)].tree)); ;}
    break;

  case 203:

/* Line 1455 of yacc.c  */
#line 414 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_BOOL_XOR,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 204:

/* Line 1455 of yacc.c  */
#line 415 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_BOOL_AND,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 205:

/* Line 1455 of yacc.c  */
#line 416 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_BOOL_OR,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 206:

/* Line 1455 of yacc.c  */
#line 417 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_EQUAL,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 207:

/* Line 1455 of yacc.c  */
#line 418 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_NOTEQUAL,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 208:

/* Line 1455 of yacc.c  */
#line 419 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_LESS,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 209:

/* Line 1455 of yacc.c  */
#line 420 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_LESS,(yyvsp[(3) - (3)].tree),(yyvsp[(1) - (3)].tree)); ;}
    break;

  case 210:

/* Line 1455 of yacc.c  */
#line 421 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_LESSEQUAL,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 211:

/* Line 1455 of yacc.c  */
#line 422 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_LESSEQUAL,(yyvsp[(3) - (3)].tree),(yyvsp[(1) - (3)].tree)); ;}
    break;

  case 212:

/* Line 1455 of yacc.c  */
#line 423 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_ADD,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 213:

/* Line 1455 of yacc.c  */
#line 424 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_SUB,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 214:

/* Line 1455 of yacc.c  */
#line 425 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_MULT,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 215:

/* Line 1455 of yacc.c  */
#line 426 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_DIV,(yyvsp[(1) - (3)].tree),(yyvsp[(3) - (3)].tree)); ;}
    break;

  case 216:

/* Line 1455 of yacc.c  */
#line 427 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_NEG,(yyvsp[(2) - (2)].tree)); ;}
    break;

  case 217:

/* Line 1455 of yacc.c  */
#line 428 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_ABS,(yyvsp[(3) - (4)].tree)); ;}
    break;

  case 218:

/* Line 1455 of yacc.c  */
#line 429 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_SQRT,(yyvsp[(3) - (4)].tree)); ;}
    break;

  case 219:

/* Line 1455 of yacc.c  */
#line 430 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_SEXT,(yyvsp[(3) - (4)].tree)); ;}
    break;

  case 220:

/* Line 1455 of yacc.c  */
#line 431 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_ZEXT,(yyvsp[(3) - (4)].tree)); ;}
    break;

  case 221:

/* Line 1455 of yacc.c  */
#line 432 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_CARRY,(yyvsp[(3) - (6)].tree),(yyvsp[(5) - (6)].tree)); ;}
    break;

  case 222:

/* Line 1455 of yacc.c  */
#line 433 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_SCARRY,(yyvsp[(3) - (6)].tree),(yyvsp[(5) - (6)].tree)); ;}
    break;

  case 223:

/* Line 1455 of yacc.c  */
#line 434 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_INT_SBORROW,(yyvsp[(3) - (6)].tree),(yyvsp[(5) - (6)].tree)); ;}
    break;

  case 224:

/* Line 1455 of yacc.c  */
#line 435 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_FLOAT2FLOAT,(yyvsp[(3) - (4)].tree)); ;}
    break;

  case 225:

/* Line 1455 of yacc.c  */
#line 436 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_INT2FLOAT,(yyvsp[(3) - (4)].tree)); ;}
    break;

  case 226:

/* Line 1455 of yacc.c  */
#line 437 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_NAN,(yyvsp[(3) - (4)].tree)); ;}
    break;

  case 227:

/* Line 1455 of yacc.c  */
#line 438 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_TRUNC,(yyvsp[(3) - (4)].tree)); ;}
    break;

  case 228:

/* Line 1455 of yacc.c  */
#line 439 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_CEIL,(yyvsp[(3) - (4)].tree)); ;}
    break;

  case 229:

/* Line 1455 of yacc.c  */
#line 440 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_FLOOR,(yyvsp[(3) - (4)].tree)); ;}
    break;

  case 230:

/* Line 1455 of yacc.c  */
#line 441 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_FLOAT_ROUND,(yyvsp[(3) - (4)].tree)); ;}
    break;

  case 231:

/* Line 1455 of yacc.c  */
#line 442 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_NEW,(yyvsp[(3) - (4)].tree)); ;}
    break;

  case 232:

/* Line 1455 of yacc.c  */
#line 443 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_NEW,(yyvsp[(3) - (6)].tree),(yyvsp[(5) - (6)].tree)); ;}
    break;

  case 233:

/* Line 1455 of yacc.c  */
#line 444 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_POPCOUNT,(yyvsp[(3) - (4)].tree)); ;}
    break;

  case 234:

/* Line 1455 of yacc.c  */
#line 445 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createOp(CPUI_SUBPIECE,new ExprTree((yyvsp[(1) - (4)].specsym)->getVarnode()),new ExprTree((yyvsp[(3) - (4)].varnode))); ;}
    break;

  case 235:

/* Line 1455 of yacc.c  */
#line 446 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createBitRange((yyvsp[(1) - (3)].specsym),0,(uint4)(*(yyvsp[(3) - (3)].i) * 8)); delete (yyvsp[(3) - (3)].i); ;}
    break;

  case 236:

/* Line 1455 of yacc.c  */
#line 447 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createBitRange((yyvsp[(1) - (6)].specsym),(uint4)*(yyvsp[(3) - (6)].i),(uint4)*(yyvsp[(5) - (6)].i)); delete (yyvsp[(3) - (6)].i), delete (yyvsp[(5) - (6)].i); ;}
    break;

  case 237:

/* Line 1455 of yacc.c  */
#line 448 "slghparse.y"
    { (yyval.tree)=slgh->pcode.createBitRange((yyvsp[(1) - (1)].bitsym)->getParentSymbol(),(yyvsp[(1) - (1)].bitsym)->getBitOffset(),(yyvsp[(1) - (1)].bitsym)->numBits()); ;}
    break;

  case 238:

/* Line 1455 of yacc.c  */
#line 449 "slghparse.y"
    { (yyval.tree) = slgh->pcode.createUserOp((yyvsp[(1) - (4)].useropsym),(yyvsp[(3) - (4)].param)); ;}
    break;

  case 239:

/* Line 1455 of yacc.c  */
#line 450 "slghparse.y"
    { if ((*(yyvsp[(3) - (4)].param)).size() < 2) { string errmsg = "Must at least two inputs to cpool"; yyerror(errmsg.c_str()); YYERROR; } (yyval.tree) = slgh->pcode.createVariadic(CPUI_CPOOLREF,(yyvsp[(3) - (4)].param)); ;}
    break;

  case 240:

/* Line 1455 of yacc.c  */
#line 452 "slghparse.y"
    { (yyval.starqual) = new StarQuality; (yyval.starqual)->size = *(yyvsp[(6) - (6)].i); delete (yyvsp[(6) - (6)].i); (yyval.starqual)->id=ConstTpl((yyvsp[(3) - (6)].spacesym)->getSpace()); ;}
    break;

  case 241:

/* Line 1455 of yacc.c  */
#line 453 "slghparse.y"
    { (yyval.starqual) = new StarQuality; (yyval.starqual)->size = 0; (yyval.starqual)->id=ConstTpl((yyvsp[(3) - (4)].spacesym)->getSpace()); ;}
    break;

  case 242:

/* Line 1455 of yacc.c  */
#line 454 "slghparse.y"
    { (yyval.starqual) = new StarQuality; (yyval.starqual)->size = *(yyvsp[(3) - (3)].i); delete (yyvsp[(3) - (3)].i); (yyval.starqual)->id=ConstTpl(slgh->getDefaultCodeSpace()); ;}
    break;

  case 243:

/* Line 1455 of yacc.c  */
#line 455 "slghparse.y"
    { (yyval.starqual) = new StarQuality; (yyval.starqual)->size = 0; (yyval.starqual)->id=ConstTpl(slgh->getDefaultCodeSpace()); ;}
    break;

  case 244:

/* Line 1455 of yacc.c  */
#line 457 "slghparse.y"
    { VarnodeTpl *sym = (yyvsp[(1) - (1)].startsym)->getVarnode(); (yyval.varnode) = new VarnodeTpl(ConstTpl(ConstTpl::j_curspace),sym->getOffset(),ConstTpl(ConstTpl::j_curspace_size)); delete sym; ;}
    break;

  case 245:

/* Line 1455 of yacc.c  */
#line 458 "slghparse.y"
    { VarnodeTpl *sym = (yyvsp[(1) - (1)].endsym)->getVarnode(); (yyval.varnode) = new VarnodeTpl(ConstTpl(ConstTpl::j_curspace),sym->getOffset(),ConstTpl(ConstTpl::j_curspace_size)); delete sym; ;}
    break;

  case 246:

/* Line 1455 of yacc.c  */
#line 459 "slghparse.y"
    { (yyval.varnode) = new VarnodeTpl(ConstTpl(ConstTpl::j_curspace),ConstTpl(ConstTpl::real,*(yyvsp[(1) - (1)].i)),ConstTpl(ConstTpl::j_curspace_size)); delete (yyvsp[(1) - (1)].i); ;}
    break;

  case 247:

/* Line 1455 of yacc.c  */
#line 460 "slghparse.y"
    { (yyval.varnode) = new VarnodeTpl(ConstTpl(ConstTpl::j_curspace),ConstTpl(ConstTpl::real,0),ConstTpl(ConstTpl::j_curspace_size)); yyerror("Parsed integer is too big (overflow)"); ;}
    break;

  case 248:

/* Line 1455 of yacc.c  */
#line 461 "slghparse.y"
    { (yyval.varnode) = (yyvsp[(1) - (1)].operandsym)->getVarnode(); (yyvsp[(1) - (1)].operandsym)->setCodeAddress(); ;}
    break;

  case 249:

/* Line 1455 of yacc.c  */
#line 462 "slghparse.y"
    { AddrSpace *spc = (yyvsp[(3) - (4)].spacesym)->getSpace(); (yyval.varnode) = new VarnodeTpl(ConstTpl(spc),ConstTpl(ConstTpl::real,*(yyvsp[(1) - (4)].i)),ConstTpl(ConstTpl::real,spc->getAddrSize())); delete (yyvsp[(1) - (4)].i); ;}
    break;

  case 250:

/* Line 1455 of yacc.c  */
#line 463 "slghparse.y"
    { (yyval.varnode) = new VarnodeTpl(ConstTpl(slgh->getConstantSpace()),ConstTpl(ConstTpl::j_relative,(yyvsp[(1) - (1)].labelsym)->getIndex()),ConstTpl(ConstTpl::real,sizeof(uintm))); (yyvsp[(1) - (1)].labelsym)->incrementRefCount(); ;}
    break;

  case 251:

/* Line 1455 of yacc.c  */
#line 464 "slghparse.y"
    { string errmsg = "Unknown jump destination: "+*(yyvsp[(1) - (1)].str); delete (yyvsp[(1) - (1)].str); yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 252:

/* Line 1455 of yacc.c  */
#line 466 "slghparse.y"
    { (yyval.varnode) = (yyvsp[(1) - (1)].specsym)->getVarnode(); ;}
    break;

  case 253:

/* Line 1455 of yacc.c  */
#line 467 "slghparse.y"
    { (yyval.varnode) = (yyvsp[(1) - (1)].varnode); ;}
    break;

  case 254:

/* Line 1455 of yacc.c  */
#line 468 "slghparse.y"
    { string errmsg = "Unknown varnode parameter: "+*(yyvsp[(1) - (1)].str); delete (yyvsp[(1) - (1)].str); yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 255:

/* Line 1455 of yacc.c  */
#line 469 "slghparse.y"
    { string errmsg = "Subtable not attached to operand: "+(yyvsp[(1) - (1)].subtablesym)->getName(); yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 256:

/* Line 1455 of yacc.c  */
#line 471 "slghparse.y"
    { (yyval.varnode) = new VarnodeTpl(ConstTpl(slgh->getConstantSpace()),ConstTpl(ConstTpl::real,*(yyvsp[(1) - (1)].i)),ConstTpl(ConstTpl::real,0)); delete (yyvsp[(1) - (1)].i); ;}
    break;

  case 257:

/* Line 1455 of yacc.c  */
#line 472 "slghparse.y"
    { (yyval.varnode) = new VarnodeTpl(ConstTpl(slgh->getConstantSpace()),ConstTpl(ConstTpl::real,0),ConstTpl(ConstTpl::real,0)); yyerror("Parsed integer is too big (overflow)"); ;}
    break;

  case 258:

/* Line 1455 of yacc.c  */
#line 473 "slghparse.y"
    { (yyval.varnode) = new VarnodeTpl(ConstTpl(slgh->getConstantSpace()),ConstTpl(ConstTpl::real,*(yyvsp[(1) - (3)].i)),ConstTpl(ConstTpl::real,*(yyvsp[(3) - (3)].i))); delete (yyvsp[(1) - (3)].i); delete (yyvsp[(3) - (3)].i); ;}
    break;

  case 259:

/* Line 1455 of yacc.c  */
#line 474 "slghparse.y"
    { (yyval.varnode) = slgh->pcode.addressOf((yyvsp[(2) - (2)].varnode),0); ;}
    break;

  case 260:

/* Line 1455 of yacc.c  */
#line 475 "slghparse.y"
    { (yyval.varnode) = slgh->pcode.addressOf((yyvsp[(4) - (4)].varnode),*(yyvsp[(3) - (4)].i)); delete (yyvsp[(3) - (4)].i); ;}
    break;

  case 261:

/* Line 1455 of yacc.c  */
#line 477 "slghparse.y"
    { (yyval.varnode) = (yyvsp[(1) - (1)].specsym)->getVarnode(); ;}
    break;

  case 262:

/* Line 1455 of yacc.c  */
#line 478 "slghparse.y"
    { string errmsg = "Unknown assignment varnode: "+*(yyvsp[(1) - (1)].str); delete (yyvsp[(1) - (1)].str); yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 263:

/* Line 1455 of yacc.c  */
#line 479 "slghparse.y"
    { string errmsg = "Subtable not attached to operand: "+(yyvsp[(1) - (1)].subtablesym)->getName(); yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 264:

/* Line 1455 of yacc.c  */
#line 481 "slghparse.y"
    { (yyval.labelsym) = (yyvsp[(2) - (3)].labelsym); ;}
    break;

  case 265:

/* Line 1455 of yacc.c  */
#line 482 "slghparse.y"
    { (yyval.labelsym) = slgh->pcode.defineLabel( (yyvsp[(2) - (3)].str) ); ;}
    break;

  case 266:

/* Line 1455 of yacc.c  */
#line 484 "slghparse.y"
    { (yyval.varnode) = (yyvsp[(1) - (1)].specsym)->getVarnode(); ;}
    break;

  case 267:

/* Line 1455 of yacc.c  */
#line 485 "slghparse.y"
    { (yyval.varnode) = slgh->pcode.addressOf((yyvsp[(2) - (2)].varnode),0); ;}
    break;

  case 268:

/* Line 1455 of yacc.c  */
#line 486 "slghparse.y"
    { (yyval.varnode) = slgh->pcode.addressOf((yyvsp[(4) - (4)].varnode),*(yyvsp[(3) - (4)].i)); delete (yyvsp[(3) - (4)].i); ;}
    break;

  case 269:

/* Line 1455 of yacc.c  */
#line 487 "slghparse.y"
    { (yyval.varnode) = new VarnodeTpl(ConstTpl(slgh->getConstantSpace()),ConstTpl(ConstTpl::real,*(yyvsp[(1) - (3)].i)),ConstTpl(ConstTpl::real,*(yyvsp[(3) - (3)].i))); delete (yyvsp[(1) - (3)].i); delete (yyvsp[(3) - (3)].i); ;}
    break;

  case 270:

/* Line 1455 of yacc.c  */
#line 488 "slghparse.y"
    { string errmsg="Unknown export varnode: "+*(yyvsp[(1) - (1)].str); delete (yyvsp[(1) - (1)].str); yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 271:

/* Line 1455 of yacc.c  */
#line 489 "slghparse.y"
    { string errmsg = "Subtable not attached to operand: "+(yyvsp[(1) - (1)].subtablesym)->getName(); yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 272:

/* Line 1455 of yacc.c  */
#line 491 "slghparse.y"
    { (yyval.famsym) = (yyvsp[(1) - (1)].valuesym); ;}
    break;

  case 273:

/* Line 1455 of yacc.c  */
#line 492 "slghparse.y"
    { (yyval.famsym) = (yyvsp[(1) - (1)].valuemapsym); ;}
    break;

  case 274:

/* Line 1455 of yacc.c  */
#line 493 "slghparse.y"
    { (yyval.famsym) = (yyvsp[(1) - (1)].contextsym); ;}
    break;

  case 275:

/* Line 1455 of yacc.c  */
#line 494 "slghparse.y"
    { (yyval.famsym) = (yyvsp[(1) - (1)].namesym); ;}
    break;

  case 276:

/* Line 1455 of yacc.c  */
#line 495 "slghparse.y"
    { (yyval.famsym) = (yyvsp[(1) - (1)].varlistsym); ;}
    break;

  case 277:

/* Line 1455 of yacc.c  */
#line 497 "slghparse.y"
    { (yyval.specsym) = (yyvsp[(1) - (1)].varsym); ;}
    break;

  case 278:

/* Line 1455 of yacc.c  */
#line 498 "slghparse.y"
    { (yyval.specsym) = (yyvsp[(1) - (1)].specsym); ;}
    break;

  case 279:

/* Line 1455 of yacc.c  */
#line 499 "slghparse.y"
    { (yyval.specsym) = (yyvsp[(1) - (1)].operandsym); ;}
    break;

  case 280:

/* Line 1455 of yacc.c  */
#line 500 "slghparse.y"
    { (yyval.specsym) = (yyvsp[(1) - (1)].startsym); ;}
    break;

  case 281:

/* Line 1455 of yacc.c  */
#line 501 "slghparse.y"
    { (yyval.specsym) = (yyvsp[(1) - (1)].endsym); ;}
    break;

  case 282:

/* Line 1455 of yacc.c  */
#line 503 "slghparse.y"
    { (yyval.str) = new string; (*(yyval.str)) += (yyvsp[(1) - (1)].ch); ;}
    break;

  case 283:

/* Line 1455 of yacc.c  */
#line 504 "slghparse.y"
    { (yyval.str) = (yyvsp[(1) - (2)].str); (*(yyval.str)) += (yyvsp[(2) - (2)].ch); ;}
    break;

  case 284:

/* Line 1455 of yacc.c  */
#line 506 "slghparse.y"
    { (yyval.biglist) = (yyvsp[(2) - (3)].biglist); ;}
    break;

  case 285:

/* Line 1455 of yacc.c  */
#line 507 "slghparse.y"
    { (yyval.biglist) = new vector<intb>; (yyval.biglist)->push_back(intb(*(yyvsp[(1) - (1)].i))); delete (yyvsp[(1) - (1)].i); ;}
    break;

  case 286:

/* Line 1455 of yacc.c  */
#line 508 "slghparse.y"
    { (yyval.biglist) = new vector<intb>; (yyval.biglist)->push_back(-intb(*(yyvsp[(2) - (2)].i))); delete (yyvsp[(2) - (2)].i); ;}
    break;

  case 287:

/* Line 1455 of yacc.c  */
#line 510 "slghparse.y"
    { (yyval.biglist) = new vector<intb>; (yyval.biglist)->push_back(intb(*(yyvsp[(1) - (1)].i))); delete (yyvsp[(1) - (1)].i); ;}
    break;

  case 288:

/* Line 1455 of yacc.c  */
#line 511 "slghparse.y"
    { (yyval.biglist) = new vector<intb>; (yyval.biglist)->push_back(-intb(*(yyvsp[(2) - (2)].i))); delete (yyvsp[(2) - (2)].i); ;}
    break;

  case 289:

/* Line 1455 of yacc.c  */
#line 512 "slghparse.y"
    { if (*(yyvsp[(1) - (1)].str)!="_") { string errmsg = "Expecting integer but saw: "+*(yyvsp[(1) - (1)].str); delete (yyvsp[(1) - (1)].str); yyerror(errmsg.c_str()); YYERROR; }
                                  (yyval.biglist) = new vector<intb>; (yyval.biglist)->push_back((intb)0xBADBEEF); delete (yyvsp[(1) - (1)].str); ;}
    break;

  case 290:

/* Line 1455 of yacc.c  */
#line 514 "slghparse.y"
    { (yyval.biglist) = (yyvsp[(1) - (2)].biglist); (yyval.biglist)->push_back(intb(*(yyvsp[(2) - (2)].i))); delete (yyvsp[(2) - (2)].i); ;}
    break;

  case 291:

/* Line 1455 of yacc.c  */
#line 515 "slghparse.y"
    { (yyval.biglist) = (yyvsp[(1) - (3)].biglist); (yyval.biglist)->push_back(-intb(*(yyvsp[(3) - (3)].i))); delete (yyvsp[(3) - (3)].i); ;}
    break;

  case 292:

/* Line 1455 of yacc.c  */
#line 516 "slghparse.y"
    { if (*(yyvsp[(2) - (2)].str)!="_") { string errmsg = "Expecting integer but saw: "+*(yyvsp[(2) - (2)].str); delete (yyvsp[(2) - (2)].str); yyerror(errmsg.c_str()); YYERROR; }
                                  (yyval.biglist) = (yyvsp[(1) - (2)].biglist); (yyval.biglist)->push_back((intb)0xBADBEEF); delete (yyvsp[(2) - (2)].str); ;}
    break;

  case 293:

/* Line 1455 of yacc.c  */
#line 519 "slghparse.y"
    { (yyval.strlist) = (yyvsp[(2) - (3)].strlist); ;}
    break;

  case 294:

/* Line 1455 of yacc.c  */
#line 520 "slghparse.y"
    { (yyval.strlist) = new vector<string>; (yyval.strlist)->push_back(*(yyvsp[(1) - (1)].str)); delete (yyvsp[(1) - (1)].str); ;}
    break;

  case 295:

/* Line 1455 of yacc.c  */
#line 522 "slghparse.y"
    { (yyval.strlist) = new vector<string>; (yyval.strlist)->push_back( *(yyvsp[(1) - (1)].str) ); delete (yyvsp[(1) - (1)].str); ;}
    break;

  case 296:

/* Line 1455 of yacc.c  */
#line 523 "slghparse.y"
    { (yyval.strlist) = (yyvsp[(1) - (2)].strlist); (yyval.strlist)->push_back(*(yyvsp[(2) - (2)].str)); delete (yyvsp[(2) - (2)].str); ;}
    break;

  case 297:

/* Line 1455 of yacc.c  */
#line 524 "slghparse.y"
    { string errmsg = (yyvsp[(2) - (2)].anysym)->getName()+": redefined"; yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 298:

/* Line 1455 of yacc.c  */
#line 526 "slghparse.y"
    { (yyval.strlist) = (yyvsp[(2) - (3)].strlist); ;}
    break;

  case 299:

/* Line 1455 of yacc.c  */
#line 528 "slghparse.y"
    { (yyval.strlist) = new vector<string>; (yyval.strlist)->push_back( *(yyvsp[(1) - (1)].str) ); delete (yyvsp[(1) - (1)].str); ;}
    break;

  case 300:

/* Line 1455 of yacc.c  */
#line 529 "slghparse.y"
    { (yyval.strlist) = new vector<string>; (yyval.strlist)->push_back( (yyvsp[(1) - (1)].anysym)->getName() ); ;}
    break;

  case 301:

/* Line 1455 of yacc.c  */
#line 530 "slghparse.y"
    { (yyval.strlist) = (yyvsp[(1) - (2)].strlist); (yyval.strlist)->push_back(*(yyvsp[(2) - (2)].str)); delete (yyvsp[(2) - (2)].str); ;}
    break;

  case 302:

/* Line 1455 of yacc.c  */
#line 531 "slghparse.y"
    { (yyval.strlist) = (yyvsp[(1) - (2)].strlist); (yyval.strlist)->push_back((yyvsp[(2) - (2)].anysym)->getName()); ;}
    break;

  case 303:

/* Line 1455 of yacc.c  */
#line 533 "slghparse.y"
    { (yyval.symlist) = (yyvsp[(2) - (3)].symlist); ;}
    break;

  case 304:

/* Line 1455 of yacc.c  */
#line 534 "slghparse.y"
    { (yyval.symlist) = new vector<SleighSymbol *>; (yyval.symlist)->push_back((yyvsp[(1) - (1)].valuesym)); ;}
    break;

  case 305:

/* Line 1455 of yacc.c  */
#line 535 "slghparse.y"
    { (yyval.symlist) = new vector<SleighSymbol *>; (yyval.symlist)->push_back((yyvsp[(1) - (1)].contextsym)); ;}
    break;

  case 306:

/* Line 1455 of yacc.c  */
#line 537 "slghparse.y"
    { (yyval.symlist) = new vector<SleighSymbol *>; (yyval.symlist)->push_back( (yyvsp[(1) - (1)].valuesym) ); ;}
    break;

  case 307:

/* Line 1455 of yacc.c  */
#line 538 "slghparse.y"
    { (yyval.symlist) = new vector<SleighSymbol *>; (yyval.symlist)->push_back((yyvsp[(1) - (1)].contextsym)); ;}
    break;

  case 308:

/* Line 1455 of yacc.c  */
#line 539 "slghparse.y"
    { (yyval.symlist) = (yyvsp[(1) - (2)].symlist); (yyval.symlist)->push_back((yyvsp[(2) - (2)].valuesym)); ;}
    break;

  case 309:

/* Line 1455 of yacc.c  */
#line 540 "slghparse.y"
    { (yyval.symlist) = (yyvsp[(1) - (2)].symlist); (yyval.symlist)->push_back((yyvsp[(2) - (2)].contextsym)); ;}
    break;

  case 310:

/* Line 1455 of yacc.c  */
#line 541 "slghparse.y"
    { string errmsg = *(yyvsp[(2) - (2)].str)+": is not a value pattern"; delete (yyvsp[(2) - (2)].str); yyerror(errmsg.c_str()); YYERROR; ;}
    break;

  case 311:

/* Line 1455 of yacc.c  */
#line 543 "slghparse.y"
    { (yyval.symlist) = (yyvsp[(2) - (3)].symlist); ;}
    break;

  case 312:

/* Line 1455 of yacc.c  */
#line 544 "slghparse.y"
    { (yyval.symlist) = new vector<SleighSymbol *>; (yyval.symlist)->push_back((yyvsp[(1) - (1)].varsym)); ;}
    break;

  case 313:

/* Line 1455 of yacc.c  */
#line 546 "slghparse.y"
    { (yyval.symlist) = new vector<SleighSymbol *>; (yyval.symlist)->push_back((yyvsp[(1) - (1)].varsym)); ;}
    break;

  case 314:

/* Line 1455 of yacc.c  */
#line 547 "slghparse.y"
    { if (*(yyvsp[(1) - (1)].str)!="_") { string errmsg = *(yyvsp[(1) - (1)].str)+": is not a varnode symbol"; delete (yyvsp[(1) - (1)].str); yyerror(errmsg.c_str()); YYERROR; }
				  (yyval.symlist) = new vector<SleighSymbol *>; (yyval.symlist)->push_back((SleighSymbol *)0); delete (yyvsp[(1) - (1)].str); ;}
    break;

  case 315:

/* Line 1455 of yacc.c  */
#line 549 "slghparse.y"
    { (yyval.symlist) = (yyvsp[(1) - (2)].symlist); (yyval.symlist)->push_back((yyvsp[(2) - (2)].varsym)); ;}
    break;

  case 316:

/* Line 1455 of yacc.c  */
#line 550 "slghparse.y"
    { if (*(yyvsp[(2) - (2)].str)!="_") { string errmsg = *(yyvsp[(2) - (2)].str)+": is not a varnode symbol"; delete (yyvsp[(2) - (2)].str); yyerror(errmsg.c_str()); YYERROR; }
                                  (yyval.symlist) = (yyvsp[(1) - (2)].symlist); (yyval.symlist)->push_back((SleighSymbol *)0); delete (yyvsp[(2) - (2)].str); ;}
    break;

  case 317:

/* Line 1455 of yacc.c  */
#line 553 "slghparse.y"
    { (yyval.param) = new vector<ExprTree *>; ;}
    break;

  case 318:

/* Line 1455 of yacc.c  */
#line 554 "slghparse.y"
    { (yyval.param) = new vector<ExprTree *>; (yyval.param)->push_back((yyvsp[(1) - (1)].tree)); ;}
    break;

  case 319:

/* Line 1455 of yacc.c  */
#line 555 "slghparse.y"
    { (yyval.param) = (yyvsp[(1) - (3)].param); (yyval.param)->push_back((yyvsp[(3) - (3)].tree)); ;}
    break;

  case 320:

/* Line 1455 of yacc.c  */
#line 557 "slghparse.y"
    { (yyval.strlist) = new vector<string>; ;}
    break;

  case 321:

/* Line 1455 of yacc.c  */
#line 558 "slghparse.y"
    { (yyval.strlist) = new vector<string>; (yyval.strlist)->push_back(*(yyvsp[(1) - (1)].str)); delete (yyvsp[(1) - (1)].str); ;}
    break;

  case 322:

/* Line 1455 of yacc.c  */
#line 559 "slghparse.y"
    { (yyval.strlist) = (yyvsp[(1) - (3)].strlist); (yyval.strlist)->push_back(*(yyvsp[(3) - (3)].str)); delete (yyvsp[(3) - (3)].str); ;}
    break;

  case 323:

/* Line 1455 of yacc.c  */
#line 561 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].spacesym); ;}
    break;

  case 324:

/* Line 1455 of yacc.c  */
#line 562 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].sectionsym); ;}
    break;

  case 325:

/* Line 1455 of yacc.c  */
#line 563 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].tokensym); ;}
    break;

  case 326:

/* Line 1455 of yacc.c  */
#line 564 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].useropsym); ;}
    break;

  case 327:

/* Line 1455 of yacc.c  */
#line 565 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].macrosym); ;}
    break;

  case 328:

/* Line 1455 of yacc.c  */
#line 566 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].subtablesym); ;}
    break;

  case 329:

/* Line 1455 of yacc.c  */
#line 567 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].valuesym); ;}
    break;

  case 330:

/* Line 1455 of yacc.c  */
#line 568 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].valuemapsym); ;}
    break;

  case 331:

/* Line 1455 of yacc.c  */
#line 569 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].contextsym); ;}
    break;

  case 332:

/* Line 1455 of yacc.c  */
#line 570 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].namesym); ;}
    break;

  case 333:

/* Line 1455 of yacc.c  */
#line 571 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].varsym); ;}
    break;

  case 334:

/* Line 1455 of yacc.c  */
#line 572 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].varlistsym); ;}
    break;

  case 335:

/* Line 1455 of yacc.c  */
#line 573 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].operandsym); ;}
    break;

  case 336:

/* Line 1455 of yacc.c  */
#line 574 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].startsym); ;}
    break;

  case 337:

/* Line 1455 of yacc.c  */
#line 575 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].endsym); ;}
    break;

  case 338:

/* Line 1455 of yacc.c  */
#line 576 "slghparse.y"
    { (yyval.anysym) = (yyvsp[(1) - (1)].bitsym); ;}
    break;



/* Line 1455 of yacc.c  */
#line 4717 "slghparse.cc"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 1675 of yacc.c  */
#line 578 "slghparse.y"


int yyerror(const char *s)

{
  slgh->reportError(s);
  return 0;
}

