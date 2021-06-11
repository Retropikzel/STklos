/*
 * gtklos.c              -- Various GTk+ wrappers for GTklos
 *
 * Copyright � 2007-2021 Erick Gallesio - I3S-CNRS/ESSI <eg@essi.fr>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 *
 *           Author: Erick Gallesio [eg@essi.fr]
 *    Creation date: 11-Aug-2007 11:38 (eg)
 * Last file update: 10-Jun-2021 14:18 (eg)
 */

#include <math.h>               /* for isnan */
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>     /* For the keysyms macros */
#include "stklos.h"
#include "gtklos-incl.c"


/* ======================================================================
 *
 *      stklos-gtklos Globals & Utilities
 *
 * ======================================================================
 */
static void error_set_property(SCM prop, SCM val, char *s)
{
  STk_error("Uncorrect type for property ~S (%s expected). Value was ~s",
            prop, s, val);
}

static void error_bad_widget(SCM obj)
{
  STk_error("bad widget ~S", obj);
}

static void error_bad_event(SCM obj)
{
  STk_error("bad event ~S", obj);
}

static void error_bad_integer(SCM obj)
{
  STk_error("bad integer ~S", obj);
}


/* ======================================================================
 *
 *      GValues ...
 *
 * ======================================================================
 */
static void Inline init_gvalue(SCM object, SCM prop, GValue *value)
{
  GParamSpec *spec;

  spec = g_object_class_find_property(
                         G_OBJECT_GET_CLASS(G_OBJECT(CPOINTER_VALUE(object))),
                         STRING_CHARS(prop));
  if (!spec)
    STk_error("Object ~S doesn't have the property ~S", object, prop);
  g_value_init(value, spec->value_type);
}

static void Inline init_gvalue_for_child(SCM container, SCM prop, GValue *value)
{
  GParamSpec* spec;

  spec = gtk_container_class_find_child_property(
                         G_OBJECT_GET_CLASS(CPOINTER_VALUE(container)),
                         STRING_CHARS(prop));
  if (!spec)
    STk_error("container ~S doesn't have the property ~S", container, prop);

  g_value_init(value, G_PARAM_SPEC_VALUE_TYPE(spec));
}


static SCM GValue2Scheme(GValue *value, SCM prop)
{
  SCM res = STk_void;

  switch (G_VALUE_TYPE(value)) {
    case G_TYPE_NONE:
      res = STk_void; break;
    case G_TYPE_CHAR:
      res = MAKE_CHARACTER((unsigned char) g_value_get_schar(value)); break;
    case G_TYPE_UCHAR:
      res = MAKE_CHARACTER((unsigned char) g_value_get_uchar(value)); break;
    case G_TYPE_BOOLEAN:
      res = MAKE_BOOLEAN(g_value_get_boolean(value)); break;
    case G_TYPE_INT:
      res = STk_long2integer((long) g_value_get_int(value)); break;
    case G_TYPE_UINT:
      res = STk_ulong2integer((long) g_value_get_uint(value));; break;
    case G_TYPE_LONG:
      res = STk_long2integer((long) g_value_get_long(value)); break;
    case G_TYPE_ULONG:
      res = STk_ulong2integer((long) g_value_get_ulong(value)); break;
    case G_TYPE_ENUM:
      res = STk_long2integer((long) g_value_get_enum(value));; break;
    case G_TYPE_FLOAT:
      res = STk_double2real((double) g_value_get_float(value)); break;
    case G_TYPE_DOUBLE:
      res = STk_double2real((double) g_value_get_double(value)); break;
    case G_TYPE_STRING: {
      char *s = (char *) g_value_get_string(value);

      res = STk_Cstring2string( (s) ? s : "");
      break;
    }
    case G_TYPE_INVALID:
    case G_TYPE_INTERFACE:
    case G_TYPE_INT64:
    case G_TYPE_UINT64:
    case G_TYPE_FLAGS:
    case G_TYPE_POINTER:
    case G_TYPE_BOXED:
    case G_TYPE_PARAM:
    case G_TYPE_OBJECT:
      STk_error("cannot convert property ~S to Scheme (%d)",
                prop, G_VALUE_TYPE(value));
    default:
      {
        /* Try to value objet to an int */
        GValue cast = {G_TYPE_INVALID};
        int n;

        g_value_init(&cast, G_TYPE_INT);

        n = g_value_transform(value, &cast);
        if (n) {
          /* STk_debug("Transformation possible"); */
          res = STk_long2integer((long) g_value_get_int(&cast));
          break;
        }
      }
      STk_error("unknown GObject type: %d",  G_VALUE_TYPE(value));
  }
  return res;
}

static void assign_GValue(GValue *value, SCM prop, SCM val)
{
  /*
  STk_debug("Setting property ~S to ~S (type = %s)\n",
     prop, val, G_VALUE_TYPE_NAME(value));
  */
  switch (G_VALUE_TYPE(value)) {
    case G_TYPE_NONE:
      break;
    case G_TYPE_CHAR:
      if (CHARACTERP(val))
        g_value_set_schar(value, (char) CHARACTER_VAL(val));
      else
        error_set_property(prop, val, "character");
      break;
    case G_TYPE_UCHAR:
      if (CHARACTERP(val))
        g_value_set_uchar(value, (unsigned char) CHARACTER_VAL(val));
        error_set_property(prop, val, "character");
      break;
    case G_TYPE_BOOLEAN:
      g_value_set_boolean(value, val != STk_false);
      break;
    case G_TYPE_INT: {
      long v = STk_integer_value(val);

      if (v != LONG_MIN)
        g_value_set_int(value, (int) v);
      else
        error_set_property(prop, val, "integer");
      break;
    }
    case G_TYPE_UINT: {
      unsigned long v = STk_uinteger_value(val);

      if (v != ULONG_MAX)
        g_value_set_uint(value, (unsigned int) v);
      else
        error_set_property(prop, val, "unsigned integer");
      break;
    }
    case G_TYPE_LONG: {
      long v = STk_integer_value(val);

      if (v != LONG_MIN)
        g_value_set_long(value, (int) v);
      else
        error_set_property(prop, val, "long integer");
      break;
    }
    case G_TYPE_ULONG: {
      unsigned long v = STk_uinteger_value(val);

      if (v != ULONG_MAX)
        g_value_set_ulong(value, (unsigned long) v);
      else
        error_set_property(prop, val, "unsigned long integer");
      break;
    }
    case G_TYPE_ENUM: {
      long v = STk_integer_value(val);

      if (v != LONG_MIN)
        g_value_set_enum(value, (int) v);
      else
        error_set_property(prop, val, "enumeration");
      break;
    }
    case G_TYPE_FLOAT: {
      double d = STk_number2double(val);

      if (!isnan(d))
        g_value_set_float(value, (float) d);
      else
        error_set_property(prop, val, "real");
      break;
    }
    case G_TYPE_DOUBLE: {
      double d = STk_number2double(val);

      if (!isnan(d))
        g_value_set_double(value, d);
      else
        error_set_property(prop, val, "real");
      break;
    }
    case G_TYPE_STRING:
      if (STRINGP(val))
        g_value_set_string(value, STRING_CHARS(val));
      else
        error_set_property(prop, val, "string");
      break;

    case G_TYPE_INVALID:
    case G_TYPE_INTERFACE:
    case G_TYPE_INT64:
    case G_TYPE_UINT64:
    case G_TYPE_FLAGS:
    case G_TYPE_POINTER:
    case G_TYPE_BOXED:
    case G_TYPE_PARAM:
    case G_TYPE_OBJECT:
      STk_error("cannot convert Scheme object ~S for property ~S", val, prop);
    default:
      {
        /* Try to value objet to an int */
        GValue cast = {G_TYPE_INVALID};
        long v = STk_integer_value(val);
        int n;

        if (v != LONG_MIN) {
          g_value_init(&cast, G_TYPE_INT);
          g_value_set_int(&cast, (int) v);

          n = g_value_transform(&cast, value);
          if (!n) {
            STk_error("cannot transform value ~S in type %s for property ~S",
                      val, G_VALUE_TYPE_NAME(value), prop);
          }
          break;
        }
      }
      STk_error("unknown GObject type: %d",  G_VALUE_TYPE(value));
  }
}

/* ======================================================================
 *
 *      stklos-gtklos Primitives
 *
 * ======================================================================
 */


/* ----------------------------------------------------------------------
 *      %gtk-get-property ...
 * ---------------------------------------------------------------------- */
DEFINE_PRIMITIVE("%gtk-get-property", gtk_get_prop, subr2,(SCM object, SCM prop))
{
  GValue value = {G_TYPE_INVALID};
  SCM res = STk_void;

  if (! CPOINTERP(object)) STk_error("bad object ~S" ,object);
  if (! STRINGP(prop))     STk_error("bad property name ~S", prop);

  init_gvalue(object, prop, &value);

  /* the Gvalue is initialized with correct type . Fill it now */
  g_object_get_property(G_OBJECT(CPOINTER_VALUE(object)),
                        STRING_CHARS(prop),
                        &value);

  res = GValue2Scheme(&value, prop);
  /* Release ressources associated to value */
  g_value_unset(&value);
  return res;
}



/* ----------------------------------------------------------------------
 *      %gtk-set-property! ...
 * ---------------------------------------------------------------------- */
DEFINE_PRIMITIVE("%gtk-set-property!", gtk_set_prop, subr3,
                 (SCM object, SCM prop, SCM val))
{
  GValue value = {G_TYPE_INVALID};

  if (! CPOINTERP(object)) STk_error("bad object ~S" ,object);
  if (! STRINGP(prop))     STk_error("bad property name ~S", prop);

  init_gvalue(object, prop, &value);

  assign_GValue(&value, prop, val);
  g_object_set_property(G_OBJECT(CPOINTER_VALUE(object)),
                        STRING_CHARS(prop),
                        &value);

  /* Release ressources associated to value */
  g_value_unset(&value);
  return STk_void;
}

//TODO: /* ----------------------------------------------------------------------
//TODO:  *      %gtk-get-size ...
//TODO:  * ---------------------------------------------------------------------- */
//TODO: DEFINE_PRIMITIVE("%gtk-get-size", gtk_get_size, subr1, (SCM obj))
//TODO: {
//TODO:   gint width, height;
//TODO:
//TODO:   if (!CPOINTERP(obj)) error_bad_widget(obj);
//TODO:
//TODO:   if (GTK_WIDGET_DRAWABLE(CPOINTER_VALUE(obj)))
//TODO:     gdk_window_get_size(GTK_WIDGET(CPOINTER_VALUE(obj))->window, &width, &height);
//TODO:   else
//TODO:     width = height = -1;
//TODO:
//TODO:   return STk_cons(MAKE_INT(width), MAKE_INT(height));
//TODO: }

/* ----------------------------------------------------------------------
 *
 *      Containers  ...
 *
 * ---------------------------------------------------------------------- */
DEFINE_PRIMITIVE("gtk-box-query-child-packing", box_query_packing, subr2,
                 (SCM box, SCM child))
{
  gboolean expand, fill;
  guint padding;
  GtkPackType pack_type;

  if (!CPOINTERP(box)) error_bad_widget(box);
  if (!CPOINTERP(child)) error_bad_widget(child);

  expand = fill = padding = pack_type = 0;
  gtk_box_query_child_packing(GTK_BOX(CPOINTER_VALUE(box)),
                              GTK_WIDGET(CPOINTER_VALUE(child)),
                              &expand,
                              &fill,
                              &padding,
                              &pack_type);

  return LIST4(MAKE_BOOLEAN(expand),
               MAKE_BOOLEAN(fill),
               MAKE_INT(padding),
               MAKE_BOOLEAN(pack_type == GTK_PACK_START));
}


/* ----------------------------------------------------------------------
 *      %gtk-get-child-property ...
 * ---------------------------------------------------------------------- */
DEFINE_PRIMITIVE("%gtk-get-child-property", gtk_get_child_prop, subr3,
                 (SCM container, SCM object, SCM prop))
{
  GValue value = {G_TYPE_INVALID};
  SCM res = STk_void;

  if (! CPOINTERP(container)) STk_error("bad container ~S" , container);
  if (! CPOINTERP(object))    STk_error("bad object ~S" , object);
  if (! STRINGP(prop))        STk_error("bad property name ~S", prop);

  init_gvalue_for_child(container, prop, &value);

  /* the Gvalue is initialized with correct type . Fill it now */
  gtk_container_child_get_property(GTK_CONTAINER(CPOINTER_VALUE(container)),
                                   GTK_WIDGET(CPOINTER_VALUE(object)),
                                   STRING_CHARS(prop),
                                   &value);

  res = GValue2Scheme(&value, prop);
  g_value_unset(&value);
  return res;
}



/*
 * FIXME: This function is not exported. Should it be? Delete it?
 *
 */
DEFINE_PRIMITIVE("gtk-box-set-child-packing", box_set_packing, subr3,
                 (SCM box, SCM child, SCM value))
{
  long pad;


  if (!CPOINTERP(box)) error_bad_widget(box);
  if (!CPOINTERP(child)) error_bad_widget(child);

  if (STk_int_length(value) != 4) STk_error("bad value ~S", value);
  pad = STk_integer_value(CAR(CDR(CDR(value))));
  if (pad == LONG_MIN) STk_error("bad padding value ~S", CAR(CDR(CDR(value))));

  gtk_box_set_child_packing(GTK_BOX(CPOINTER_VALUE(box)),
                            GTK_WIDGET(CPOINTER_VALUE(child)),
                            (gboolean) 0, //(CAR(value) != STk_false),
                            (gboolean) ~0, // (CAR(CDR(value)) != STk_false),
                            (guint) pad,
                            ((CAR(CDR((CDR(CDR(value))))) == STk_true)?
                             GTK_PACK_START: GTK_PACK_END));
  return STk_void;
}

/*
 * Children of a container computation
 */
static void cont_children_helper(gpointer p, gpointer data)
{
  SCM *l = (SCM*) data;

  *l = STk_cons(STk_make_Cpointer(p, STk_void, STk_false), *l);
}



DEFINE_PRIMITIVE("%container-children", cont_children, subr1, (SCM w))
{
  GList *gl;
  SCM l = STk_nil;

  if (!CPOINTERP(w)) error_bad_widget(w);

  gl = gtk_container_get_children(GTK_CONTAINER(CPOINTER_VALUE(w)));
  g_list_foreach(gl, cont_children_helper, &l);
  g_list_free(gl);

  return l;
}

//TODO: /* ----------------------------------------------------------------------
//TODO:  *
//TODO:  *      Events  ...
//TODO:  *
//TODO:  * ---------------------------------------------------------------------- */
//TODO: DEFINE_PRIMITIVE("%event-type", event_type, subr1, (SCM event))
//TODO: {
//TODO:   GdkEvent *ev;
//TODO:
//TODO:   if (!CPOINTERP(event)) error_bad_event(event);
//TODO:   ev = CPOINTER_VALUE(event);
//TODO:
//TODO:   switch (((GdkEventAny *) ev)->type) {
//TODO:     case GDK_NOTHING           : return STk_intern("NOTHING");
//TODO:     case GDK_DELETE            : return STk_intern("DELETE");
//TODO:     case GDK_DESTROY           : return STk_intern("DESTROY");
//TODO:     case GDK_EXPOSE            : return STk_intern("EXPOSE");
//TODO:     case GDK_MOTION_NOTIFY     : return STk_intern("MOTION");
//TODO:     case GDK_BUTTON_PRESS      :
//TODO:     case GDK_2BUTTON_PRESS     :
//TODO:     case GDK_3BUTTON_PRESS     : return STk_intern("PRESS");
//TODO:     case GDK_BUTTON_RELEASE    : return STk_intern("RELEASE");
//TODO:     case GDK_KEY_PRESS         : return STk_intern("KEY-PRESS");
//TODO:     case GDK_KEY_RELEASE       : return STk_intern("KEY-RELEASE");
//TODO:     case GDK_ENTER_NOTIFY      : return STk_intern("ENTER");
//TODO:     case GDK_LEAVE_NOTIFY      : return STk_intern("LEAVE");
//TODO:     case GDK_FOCUS_CHANGE      : return STk_intern(((GdkEventFocus *) ev)->in ?
//TODO:                                                       "FOCUS-IN" :
//TODO:                                                       "FOCUS-OUT");
//TODO:     case GDK_CONFIGURE         : return STk_intern("CONFIGURE");
//TODO:     case GDK_MAP               : return STk_intern("MAP");
//TODO:     case GDK_UNMAP             : return STk_intern("UNMAP");
//TODO:     case GDK_PROPERTY_NOTIFY   : return STk_intern("PROPERTY");
//TODO:     case GDK_SELECTION_CLEAR   : return STk_intern("SELECTION-CLEAR");
//TODO:     case GDK_SELECTION_REQUEST : return STk_intern("SELECTION-REQUEST");
//TODO:     case GDK_SELECTION_NOTIFY  : return STk_intern("SELECTION");
//TODO:     case GDK_PROXIMITY_IN      : return STk_intern("PROXIMITY-IN");
//TODO:     case GDK_PROXIMITY_OUT     : return STk_intern("PROXIMITY-OUT");
//TODO:     case GDK_DRAG_ENTER        : return STk_intern("DRAG-ENTER");
//TODO:     case GDK_DRAG_LEAVE        : return STk_intern("DRAG-LEAVE");
//TODO:     case GDK_DRAG_MOTION       : return STk_intern("DRAG-MOTION");
//TODO:     case GDK_DRAG_STATUS       : return STk_intern("DRAG-STATUS");
//TODO:     case GDK_DROP_START        : return STk_intern("DROP-START");
//TODO:     case GDK_DROP_FINISHED     : return STk_intern("DROP-FINISHED");
//TODO:     case GDK_CLIENT_EVENT      : return STk_intern("CLIENT-EVENT");
//TODO:     case GDK_VISIBILITY_NOTIFY : return STk_intern("VISIBILITY");
//TODO:     case GDK_NO_EXPOSE         : return STk_intern("NO-EXPOSE");
//TODO:     default:                     return STk_void;
//TODO:   }
//TODO: }
//TODO:
//TODO:
//TODO: DEFINE_PRIMITIVE("%event-x", event_x, subr1, (SCM event))
//TODO: {
//TODO:   GdkEvent *ev;
//TODO:
//TODO:   if (!CPOINTERP(event)) error_bad_event(event);
//TODO:   ev = CPOINTER_VALUE(event);
//TODO:
//TODO:   switch(((GdkEventAny *) ev)->type) {
//TODO:     case GDK_ENTER_NOTIFY:
//TODO:     case GDK_LEAVE_NOTIFY:   return STk_double2real(((GdkEventCrossing *)  ev)->x);
//TODO:     case GDK_BUTTON_PRESS:
//TODO:     case GDK_2BUTTON_PRESS:
//TODO:     case GDK_3BUTTON_PRESS:
//TODO:     case GDK_BUTTON_RELEASE: return STk_double2real(((GdkEventButton *)    ev)->x);
//TODO:     case GDK_MOTION_NOTIFY:  return STk_double2real(((GdkEventMotion *)    ev)->x);
//TODO:     case GDK_CONFIGURE:      return MAKE_INT(((GdkEventConfigure *) ev)->x);
//TODO:     default:                 return MAKE_INT(-1);
//TODO:   }
//TODO: }
//TODO:
//TODO:
//TODO: DEFINE_PRIMITIVE("%event-y", event_y, subr1, (SCM event))
//TODO: {
//TODO:   GdkEvent *ev;
//TODO:
//TODO:   if (!CPOINTERP(event)) error_bad_event(event);
//TODO:   ev = CPOINTER_VALUE(event);
//TODO:
//TODO:   switch(((GdkEventAny *) ev)->type) {
//TODO:     case GDK_ENTER_NOTIFY:
//TODO:     case GDK_LEAVE_NOTIFY:   return STk_double2real(((GdkEventCrossing *)  ev)->y);
//TODO:     case GDK_BUTTON_PRESS:
//TODO:     case GDK_2BUTTON_PRESS:
//TODO:     case GDK_3BUTTON_PRESS:
//TODO:     case GDK_BUTTON_RELEASE: return STk_double2real(((GdkEventButton *)    ev)->y);
//TODO:     case GDK_MOTION_NOTIFY:  return STk_double2real(((GdkEventMotion *)    ev)->y);
//TODO:     case GDK_CONFIGURE:      return MAKE_INT(((GdkEventConfigure *) ev)->y);
//TODO:     default:                 return MAKE_INT(-1);
//TODO:   }
//TODO: }
//TODO:
//TODO:
//TODO: DEFINE_PRIMITIVE("%event-char", event_char, subr1, (SCM event))
//TODO: {
//TODO:   GdkEvent *ev;
//TODO:   int keyval;
//TODO:
//TODO:   if (!CPOINTERP(event)) error_bad_event(event);
//TODO:   ev = CPOINTER_VALUE(event);
//TODO:
//TODO:   switch (((GdkEventAny *) ev)->type) {
//TODO:     case GDK_KEY_PRESS:
//TODO:       keyval = ((GdkEventKey *)ev)->keyval;
//TODO:       switch (keyval) {
//TODO:         case GDK_Return: return MAKE_CHARACTER('\n');
//TODO:         case GDK_Tab:    return MAKE_CHARACTER('\t');
//TODO:         default:         keyval = (keyval < 0xff) ? keyval: 0;;
//TODO:                          return MAKE_CHARACTER(keyval);
//TODO:       }
//TODO:     default:
//TODO:       return MAKE_CHARACTER(0);
//TODO:   }
//TODO: }
//TODO:
//TODO: DEFINE_PRIMITIVE("%event-keysym", event_keysym, subr1, (SCM event))
//TODO: {
//TODO:   GdkEvent *ev;
//TODO:   int keyval;
//TODO:
//TODO:   if (!CPOINTERP(event)) error_bad_event(event);
//TODO:   ev = CPOINTER_VALUE(event);
//TODO:
//TODO:   switch (((GdkEventAny *) ev)->type) {
//TODO:     case GDK_KEY_PRESS:
//TODO:       switch (keyval = ((GdkEventKey *)ev)->keyval) {
//TODO:         case GDK_Return: return MAKE_CHARACTER('\n');
//TODO:         case GDK_Tab:    return MAKE_CHARACTER('\t');
//TODO:         default:         return MAKE_INT(keyval);
//TODO:       }
//TODO:     default:
//TODO:       return STk_void;
//TODO:   }
//TODO: }
//TODO:
//TODO: DEFINE_PRIMITIVE("%event-modifiers", event_modifiers,  subr1, (SCM event))
//TODO: {
//TODO:   GdkEvent *ev;
//TODO:   SCM res = STk_nil;
//TODO:   int modifiers;
//TODO:
//TODO:   if (!CPOINTERP(event)) error_bad_event(event);
//TODO:   ev = CPOINTER_VALUE(event);
//TODO:
//TODO:   switch (((GdkEventAny *) ev)->type) {
//TODO:       case GDK_ENTER_NOTIFY:
//TODO:       case GDK_LEAVE_NOTIFY:
//TODO:         modifiers = ((GdkEventCrossing *)ev)->state;
//TODO:         break;
//TODO:       case GDK_BUTTON_PRESS:
//TODO:       case GDK_2BUTTON_PRESS:
//TODO:       case GDK_3BUTTON_PRESS:
//TODO:       case GDK_BUTTON_RELEASE:
//TODO:          modifiers = ((GdkEventButton *) ev)->state;
//TODO:          break;
//TODO:       case GDK_MOTION_NOTIFY:
//TODO:          modifiers = ((GdkEventMotion *) ev)->state;
//TODO:          break;
//TODO:       case GDK_KEY_PRESS:
//TODO:          modifiers = ((GdkEventKey *) ev)->state;
//TODO:          break;
//TODO:       default:
//TODO:          modifiers = 0;
//TODO:    }
//TODO:
//TODO:    if (modifiers & GDK_SHIFT_MASK)
//TODO:       res = STk_cons(STk_intern("shift"), res);
//TODO:
//TODO:    if (modifiers & GDK_LOCK_MASK)
//TODO:       res = STk_cons(STk_intern("lock"), res);
//TODO:
//TODO:    if (modifiers & GDK_CONTROL_MASK)
//TODO:       res = STk_cons(STk_intern("control"), res);
//TODO:
//TODO:    if (modifiers & GDK_MOD1_MASK)
//TODO:       res = STk_cons(STk_intern("mod1"), res);
//TODO:
//TODO:    if (modifiers & GDK_MOD2_MASK)
//TODO:       res = STk_cons(STk_intern("mod2"), res);
//TODO:
//TODO:    if (modifiers & GDK_MOD3_MASK)
//TODO:       res = STk_cons(STk_intern("mod3"), res);
//TODO:
//TODO:    if (modifiers & GDK_MOD4_MASK)
//TODO:       res = STk_cons(STk_intern("mod4"), res);
//TODO:
//TODO:    if (modifiers & GDK_MOD5_MASK)
//TODO:       res = STk_cons(STk_intern("mod5"), res);
//TODO:
//TODO:    return res;
//TODO: }
//TODO:
//TODO: DEFINE_PRIMITIVE("%event-button", event_button, subr1, (SCM event))
//TODO: {
//TODO:   GdkEvent *ev;
//TODO:
//TODO:   if (!CPOINTERP(event)) error_bad_event(event);
//TODO:   ev = CPOINTER_VALUE(event);
//TODO:
//TODO:   switch (((GdkEventAny *)ev)->type) {
//TODO:     case GDK_BUTTON_PRESS:
//TODO:     case GDK_2BUTTON_PRESS:
//TODO:     case GDK_3BUTTON_PRESS:
//TODO:     case GDK_BUTTON_RELEASE: return MAKE_INT(((GdkEventButton *)ev)->button);
//TODO:     case GDK_MOTION_NOTIFY: {
//TODO:          int state = ((GdkEventMotion *)ev)->state;
//TODO:          if( state & GDK_BUTTON1_MASK )
//TODO:             return MAKE_INT(1);
//TODO:          if( state & GDK_BUTTON2_MASK )
//TODO:             return MAKE_INT(2);
//TODO:          if( state & GDK_BUTTON3_MASK )
//TODO:             return MAKE_INT(3);
//TODO:          return MAKE_INT(0);
//TODO:       }
//TODO:     default: return MAKE_INT(0);
//TODO:   }
//TODO: }

/* ----------------------------------------------------------------------
 *
 *      Colors  ...
 *
 * ---------------------------------------------------------------------- */
DEFINE_PRIMITIVE("%string->color", string2color, subr1, (SCM str))
{
  GdkRGBA *color;

  if (!STRINGP(str)) STk_error("bad string ~S", str);

  color = STk_must_malloc_atomic(sizeof(GdkRGBA));
  if (!gdk_rgba_parse(color, STRING_CHARS(str)))
    STk_error("color ~S specification cannot be parsed ", str);
  return STk_make_Cpointer(color, STk_void, STk_false);
}


DEFINE_PRIMITIVE("%color->string", color2string, subr1, (SCM obj))
{
  if (!CPOINTERP(obj)) STk_error("bad color description ~S", obj);

  return STk_Cstring2string(gdk_rgba_to_string(CPOINTER_VALUE(obj)));
}

/* ----------------------------------------------------------------------
 *
 *      Dialogs  ...
 *
 * ---------------------------------------------------------------------- */
DEFINE_PRIMITIVE("%dialog-vbox", dialog_vbox, subr1, (SCM obj))
{
  if (!CPOINTERP(obj)) STk_error("bad dialog ~S", obj);

  /* In 2.14, we no more need to query the vbox field of the dialog
   * since the function "gtk_dialog_get_content_area" has been
   * defined. However, this version of GTK+ is still experimental
   */
  //  return STk_make_Cpointer(((GtkDialog *) CPOINTER_VALUE(obj))->vbox,
  //                       STk_void,
  //                       STk_false);

  return STk_make_Cpointer(
             gtk_dialog_get_content_area(GTK_DIALOG(CPOINTER_VALUE(obj))),
             STk_void,
             STk_false);
}

/* ----------------------------------------------------------------------
 *
 *      Timeout  ...
 *
 * ---------------------------------------------------------------------- */
static gboolean do_timeout_call(gpointer proc)
{
  return STk_C_apply((SCM) proc, 0) != STk_false;
}


DEFINE_PRIMITIVE("timeout", timeout, subr2, (SCM delay, SCM proc))
{
  long val = STk_integer_value(delay);

  if (val == LONG_MIN) error_bad_integer(delay);
  if (STk_procedurep(proc) == STk_false) STk_error("bad procedure ~S", proc);

  return STk_long2integer((long) g_timeout_add(val, do_timeout_call, proc));
}

DEFINE_PRIMITIVE("when-idle", when_idle, subr1, (SCM proc))
{
  if (STk_procedurep(proc) == STk_false) STk_error("bad procedure ~S", proc);

  return STk_long2integer((long) g_idle_add(do_timeout_call, proc));
}

DEFINE_PRIMITIVE("kill-idle", kill_idle, subr1, (SCM id))
{
  long val = STk_integer_value(id);

  if (val == LONG_MIN) error_bad_integer(id);
  return MAKE_BOOLEAN(g_source_remove(val));
}


/* ======================================================================
 *
 *      Module stklos-gtklos starts here
 *
 * ======================================================================
 */


MODULE_ENTRY_START("stklos-gtklos") {

  SCM gtklos_module;

  /* Create a new module named "stklos-gtklos" */
  gtklos_module = STk_create_module(STk_intern("GTKLOS"));

  /* Add new primitives */
  ADD_PRIMITIVE_IN_MODULE(gtk_get_prop, gtklos_module);
  ADD_PRIMITIVE_IN_MODULE(gtk_set_prop, gtklos_module);
  //TODO:  ADD_PRIMITIVE_IN_MODULE(gtk_get_size, gtklos_module);

  ADD_PRIMITIVE_IN_MODULE(gtk_get_child_prop, gtklos_module);

  ADD_PRIMITIVE_IN_MODULE(box_query_packing, gtklos_module);
  ADD_PRIMITIVE_IN_MODULE(box_set_packing, gtklos_module);
  ADD_PRIMITIVE_IN_MODULE(cont_children, gtklos_module);

//TODO:   ADD_PRIMITIVE_IN_MODULE(event_type, gtklos_module);
//TODO:   ADD_PRIMITIVE_IN_MODULE(event_x, gtklos_module);
//TODO:   ADD_PRIMITIVE_IN_MODULE(event_y, gtklos_module);
//TODO:   ADD_PRIMITIVE_IN_MODULE(event_char, gtklos_module);
//TODO:   ADD_PRIMITIVE_IN_MODULE(event_modifiers, gtklos_module);
//TODO:   ADD_PRIMITIVE_IN_MODULE(event_button, gtklos_module);

  ADD_PRIMITIVE_IN_MODULE(string2color, gtklos_module);
  ADD_PRIMITIVE_IN_MODULE(color2string, gtklos_module);

  ADD_PRIMITIVE_IN_MODULE(dialog_vbox, gtklos_module);

  ADD_PRIMITIVE_IN_MODULE(timeout, gtklos_module);
  ADD_PRIMITIVE_IN_MODULE(when_idle, gtklos_module);
  ADD_PRIMITIVE_IN_MODULE(kill_idle, gtklos_module);

  /* Execute Scheme code */
  STk_execute_C_bytecode(__module_consts, __module_code);

} MODULE_ENTRY_END

DEFINE_MODULE_INFO
