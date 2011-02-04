/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 *
 * Data structure for storing a message of protobuf data.
 */

#include "upb_msg.h"
#include "upb_decoder.h"
#include "upb_strstream.h"

static void upb_elem_free(upb_value v, upb_fielddef *f) {
  switch(f->type) {
    case UPB_TYPE(MESSAGE):
    case UPB_TYPE(GROUP):
      _upb_msg_free(upb_value_getmsg(v), upb_downcast_msgdef(f->def));
      break;
    case UPB_TYPE(STRING):
    case UPB_TYPE(BYTES):
      _upb_string_free(upb_value_getstr(v));
      break;
    default:
      abort();
  }
}

static void upb_elem_unref(upb_value v, upb_fielddef *f) {
  assert(upb_elem_ismm(f));
  upb_atomic_refcount_t *refcount = upb_value_getrefcount(v);
  if (refcount && upb_atomic_unref(refcount))
    upb_elem_free(v, f);
}

static void upb_field_free(upb_value v, upb_fielddef *f) {
  if (upb_isarray(f)) {
    _upb_array_free(upb_value_getarr(v), f);
  } else {
    upb_elem_free(v, f);
  }
}

static void upb_field_unref(upb_value v, upb_fielddef *f) {
  assert(upb_field_ismm(f));
  upb_atomic_refcount_t *refcount = upb_value_getrefcount(v);
  if (refcount && upb_atomic_unref(refcount))
    upb_field_free(v, f);
}

upb_msg *upb_msg_new(upb_msgdef *md) {
  upb_msg *msg = malloc(md->size);
  // Clear all set bits and cached pointers.
  memset(msg, 0, md->size);
  upb_atomic_refcount_init(&msg->refcount, 1);
  return msg;
}

void _upb_msg_free(upb_msg *msg, upb_msgdef *md) {
  // Need to release refs on all sub-objects.
  upb_msg_iter i;
  for(i = upb_msg_begin(md); !upb_msg_done(i); i = upb_msg_next(md, i)) {
    upb_fielddef *f = upb_msg_iter_field(i);
    upb_valueptr p = _upb_msg_getptr(msg, f);
    upb_valuetype_t type = upb_field_valuetype(f);
    if (upb_field_ismm(f)) upb_field_unref(upb_value_read(p, type), f);
  }
  free(msg);
}

INLINE void upb_msg_sethas(upb_msg *msg, upb_fielddef *f) {
  msg->data[f->field_index/8] |= (1 << (f->field_index % 8));
}


upb_array *upb_array_new(void) {
  upb_array *arr = malloc(sizeof(*arr));
  upb_atomic_refcount_init(&arr->refcount, 1);
  arr->size = 0;
  arr->len = 0;
  arr->elements._void = NULL;
  return arr;
}

void _upb_array_free(upb_array *arr, upb_fielddef *f) {
  if (upb_elem_ismm(f)) {
    // Need to release refs on sub-objects.
    upb_valuetype_t type = upb_elem_valuetype(f);
    for (upb_arraylen_t i = 0; i < arr->size; i++) {
      upb_valueptr p = _upb_array_getptr(arr, f, i);
      upb_elem_unref(upb_value_read(p, type), f);
    }
  }
  if (arr->elements._void) free(arr->elements._void);
  free(arr);
}

void upb_msg_register_handlers(upb_msg *msg, upb_msgdef *md,
                               upb_handlers *handlers, bool merge) {
  static upb_handlerset handlerset = {
  }
}
