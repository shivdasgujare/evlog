#ifndef YYERRCODE
#define YYERRCODE 256
#endif

#define TK_DELIMITER 257
#define TK_TYPENAME 258
#define TK_NAME 259
#define TK_INTEGER 260
#define TK_DOUBLE 261
#define TK_STRING 262
#define TK_CHAR 263
#define TK_WSTRING 264
#define TK_WCHAR 265
#define TK_FORMAT 266
#define TK_FORMAT_STRING 267
#define TK_FORMAT_WSTRING 268
#define TK_ALIGNED 269
#define TK_ATTRIBUTES 270
#define TK_CONST 271
#define TK_DESCRIPTION 272
#define TK_DEFAULT 273
#define TK_EVENT_TYPE 274
#define TK_FACILITY 275
#define TK_IMPORT 276
#define TK_STRUCT 277
#define TK_TYPEDEF 278
#define TK_REDIRECT 279
#define TK_TYCHAR 280
#define TK_TYDOUBLE 281
#define TK_TYINT 282
#define TK_TYLONG 283
#define TK_TYSHORT 284
#define TK_TYSIGNED 285
#define TK_TYUNSIGNED 286
#define ERRTOK 287
typedef union {
	int		ival;
	unsigned int	uival;
	unsigned long	ulval;
	long		lval;
	double		dval;
	char		*sval;
	char		**sarrval;
	int		cval;	/* character */
	wchar_t		wcval;
	wchar_t		*wsval;
	tmpl_data_type_t	*tyval;		/* attribute's data type */
	tmpl_dimension_t	*dimval;	/* attribute's dimension */
	tmpl_type_and_value_t	*valval;	/* initializer */
	tmpl_delimiter_t	*delimval;	/* attribute's delimiter */
	tmpl_redirection_t	*redir;
	struct redirectedAttribute *rdatt;
	evl_list_t	*listval;
	posix_log_facility_t facval;
} YYSTYPE;
extern YYSTYPE ttlval;
