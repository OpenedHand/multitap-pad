#include "gtkimcontextmultipress.h"

typedef struct _MbIMContext MbIMContext;
typedef struct _MbIMContextClass MbIMContextClass;

struct _MbIMContext
{
  GtkImContextMultipress context;
};

struct _MbIMContextClass
{
  GtkImContextMultipressClass parent_class;
};

void mb_im_context_register_type (GTypeModule *module);

GtkIMContext *mb_im_context_new (void);
