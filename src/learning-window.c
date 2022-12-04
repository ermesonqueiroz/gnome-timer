/* learning-window.c
 *
 * Copyright 2022 Ermeson Sampaio
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config.h"

#include "learning-window.h"

struct _LearningWindow
{
  AdwApplicationWindow  parent_instance;

  /* Template widgets */
  AdwViewStack        *stack;
  GtkAdjustment       *adjustment_hours;
  GtkAdjustment       *adjustment_minutes;
  GtkAdjustment       *adjustment_seconds;
  GtkLabel            *countdown_hours_label;
  GtkLabel            *countdown_minutes_label;
  GtkLabel            *countdown_seconds_label;
  AdwViewStack        *timer_face_controls_stack;
  GtkActionable       *actions_box;
  GtkSpinButton       *h_spinbutton;
  GtkSpinButton       *m_spinbutton;
  GtkSpinButton       *s_spinbutton;
};

G_DEFINE_FINAL_TYPE (LearningWindow, learning_window, ADW_TYPE_APPLICATION_WINDOW)

static void
update_timer_countdown (LearningWindow *self,
                        int hours,
                        int minutes,
                        int seconds)
{
  gtk_label_set_text (self->countdown_hours_label,   g_strdup_printf("%02d", hours));
  gtk_label_set_text (self->countdown_minutes_label, g_strdup_printf("%02d", minutes));
  gtk_label_set_text (self->countdown_seconds_label, g_strdup_printf("%02d", seconds));
}

static void
reset_adjustments_values (LearningWindow *self)
{
  gtk_adjustment_set_value (self->adjustment_hours, 0);
  gtk_adjustment_set_value (self->adjustment_minutes, 0);
  gtk_adjustment_set_value (self->adjustment_seconds, 0);
}

static void
set_adjustments_values (LearningWindow *self,
                        int hours,
                        int minutes,
                        int seconds)
{
  gtk_adjustment_set_value (self->adjustment_hours,   hours);
  gtk_adjustment_set_value (self->adjustment_minutes, minutes);
  gtk_adjustment_set_value (self->adjustment_seconds, seconds);
}

static void
timer_notify (LearningWindow  *self)
{
  GNotification *notification = g_notification_new ("Time is up!");
  g_notification_set_body (notification, "Timer countdown finished");
  g_application_send_notification (g_application_get_default (),
                                   NULL,
                                   notification);

  int hours   = gtk_adjustment_get_value (self->adjustment_hours);
  int minutes = gtk_adjustment_get_value (self->adjustment_minutes);
  int seconds = gtk_adjustment_get_value (self->adjustment_seconds);

  update_timer_countdown (self,
                          hours,
                          minutes,
                          seconds);

  adw_view_stack_set_visible_child_name (self->timer_face_controls_stack, "done");
}

static gboolean
timer_callback (LearningWindow  *self)
{

  if (g_strcmp0 (adw_view_stack_get_visible_child_name (self->timer_face_controls_stack), "paused") == 0)
    return G_SOURCE_CONTINUE;

  gint64 hours   = g_ascii_strtoll (gtk_label_get_text (self->countdown_hours_label),   NULL, 10);
  gint64 minutes = g_ascii_strtoll (gtk_label_get_text (self->countdown_minutes_label), NULL, 10);
  gint64 seconds = g_ascii_strtoll (gtk_label_get_text (self->countdown_seconds_label), NULL, 10);

  int total_in_seconds = (((hours * 60 + minutes) * 60) + seconds) - 1;

  if (total_in_seconds < 0) return G_SOURCE_REMOVE;

  update_timer_countdown (self,
                          (int) ((gint64) total_in_seconds / 60) / 60,
                          (int) ((gint64) total_in_seconds / 60) % 60,
                          (int) total_in_seconds % 60);

  return G_SOURCE_CONTINUE;
}

static void
reset_timer (GtkButton      *button,
             LearningWindow *self)
{
  int hours   = gtk_adjustment_get_value (self->adjustment_hours);
  int minutes = gtk_adjustment_get_value (self->adjustment_minutes);
  int seconds = gtk_adjustment_get_value (self->adjustment_seconds);

  update_timer_countdown (self,
                          hours,
                          minutes,
                          seconds);
}

static void
pause_timer (GtkButton      *button,
             LearningWindow *self)
{
  adw_view_stack_set_visible_child_name (self->timer_face_controls_stack, "paused");
}

static void
destroy_timer (GtkButton      *button,
               LearningWindow *self)
{
  reset_adjustments_values (self);
  adw_view_stack_set_visible_child_name (self->stack, "setup");
}

static void
resume_timer (GtkButton      *button,
              LearningWindow *self)
{
  adw_view_stack_set_visible_child_name (self->timer_face_controls_stack, "running");
}

static void
start_timer (GtkButton       *button,
             LearningWindow  *self)
{
  adw_view_stack_set_visible_child_name (self->stack, "face");
  adw_view_stack_set_visible_child_name (self->timer_face_controls_stack, "running");

  int hours   = gtk_adjustment_get_value (self->adjustment_hours);
  int minutes = gtk_adjustment_get_value (self->adjustment_minutes);
  int seconds = gtk_adjustment_get_value (self->adjustment_seconds);

  update_timer_countdown (self, hours, minutes, seconds);

  g_timeout_add_seconds_full (G_PRIORITY_HIGH,
                              1,
                              G_SOURCE_FUNC (timer_callback),
                              self,
                              (GDestroyNotify) timer_notify);
}

static void
set_duration (GSimpleAction   *self,
              GVariant        *parameter,
              LearningWindow  *window)
{
  int total_minutes = g_variant_get_int32 (parameter);
  int hours = total_minutes / 60;
  int minutes = total_minutes % 60;

  set_adjustments_values (window, hours, minutes, 0);
}

static gboolean
format_spin_buttons (GtkSpinButton  *button,
                     LearningWindow *self)
{
  GtkAdjustment *adjustment = gtk_spin_button_get_adjustment (button);
  int value = gtk_adjustment_get_value (adjustment);
  char *text = g_strdup_printf ("%02d", value);
  gtk_editable_set_text (GTK_EDITABLE (button), text);
  g_free (text);

  return TRUE;
}

static void
learning_window_class_init (LearningWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/ermesonqueiroz/learning/learning-window.ui");
  gtk_widget_class_bind_template_child (widget_class, LearningWindow, stack);
  gtk_widget_class_bind_template_child (widget_class, LearningWindow, adjustment_hours);
  gtk_widget_class_bind_template_child (widget_class, LearningWindow, adjustment_minutes);
  gtk_widget_class_bind_template_child (widget_class, LearningWindow, adjustment_seconds);
  gtk_widget_class_bind_template_child (widget_class, LearningWindow, countdown_hours_label);
  gtk_widget_class_bind_template_child (widget_class, LearningWindow, countdown_minutes_label);
  gtk_widget_class_bind_template_child (widget_class, LearningWindow, countdown_seconds_label);
  gtk_widget_class_bind_template_child (widget_class, LearningWindow, timer_face_controls_stack);
  gtk_widget_class_bind_template_child (widget_class, LearningWindow, actions_box);
  gtk_widget_class_bind_template_child (widget_class, LearningWindow, h_spinbutton);
  gtk_widget_class_bind_template_child (widget_class, LearningWindow, m_spinbutton);
  gtk_widget_class_bind_template_child (widget_class, LearningWindow, s_spinbutton);

  gtk_widget_class_bind_template_callback (widget_class, start_timer);
  gtk_widget_class_bind_template_callback (widget_class, resume_timer);
  gtk_widget_class_bind_template_callback (widget_class, destroy_timer);
  gtk_widget_class_bind_template_callback (widget_class, pause_timer);
  gtk_widget_class_bind_template_callback (widget_class, reset_timer);
  gtk_widget_class_bind_template_callback (widget_class, format_spin_buttons);
}

static void
learning_window_init (LearningWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  GSimpleActionGroup *actions = g_simple_action_group_new ();
  GVariantType *duration_type = g_variant_type_new("i");
  GSimpleAction *set_duration_action = g_simple_action_new("set-duration", duration_type);

  g_signal_connect (set_duration_action,
                    "activate",
                    G_CALLBACK (set_duration),
                    self);

  g_action_map_add_action (G_ACTION_MAP (actions), G_ACTION (set_duration_action));
  gtk_widget_insert_action_group (GTK_WIDGET (self->actions_box), "timer-setup", G_ACTION_GROUP (actions));
}
