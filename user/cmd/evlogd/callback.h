/* 
 * Define the callback command - 
 */
#define CB_WR_NOTIFY		0
#define CB_MK_EVL_REC		1
#define CB_UNLOAD_PLUGIN	2
#define CB_PLUGIN_FD_SET	3
#define CB_PLUGIN_FD_UNSET	4

typedef int (*evl_callback_t) (int cmd, const char *data1, const char *data2);

typedef int (* BE_FUNC_T)(const char * data1, const char * data2, evl_callback_t cb_func);
typedef int (* BE_INIT_T)(evl_callback_t cb_func);

typedef struct {
  BE_INIT_T init; 
  BE_FUNC_T process;
} be_desc_t;


extern int evl_callback(int cmd, const char *data1, const char *data2);
//extern evl_callback_t evl_callback;
