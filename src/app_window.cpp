/*
     - GNOME editor of ACL file permissions.
    Copyright (C) 2022 Roger Ferrer Ibáñez

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
   USA
*/

#include "eiciel/app_window.h"
#include "config.h"
#include "eiciel/acl_editor_controller.h"
#include "eiciel/acl_editor_widget.h"
#include "eiciel/application.h"
#include "eiciel/xattr_editor_controller.h"
#include "eiciel/xattr_editor_widget.h"
#include <gtkmm/filechoosernative.h>
#include <stdexcept>

namespace eiciel {

// static
AppWindow *AppWindow::create(Application::EditMode mode) {
  // Load the Builder file and instantiate its widgets.
  auto refBuilder = Gtk::Builder::create_from_resource(
      "/org/roger_ferrer/eiciel/app_window.ui");

  ACLEditorController *acl_editor_controller = new ACLEditorController();
  XAttrEditorController *xattr_editor_controller = new XAttrEditorController();
  auto window = Gtk::Builder::get_widget_derived<AppWindow>(
      refBuilder, "app_window", acl_editor_controller, xattr_editor_controller,
      mode);
  if (!window)
    throw std::runtime_error("No \"app_window\" object app_in app_window.ui");

  return window;
}

AppWindow::AppWindow(BaseObjectType *cobject,
                     const Glib::RefPtr<Gtk::Builder> &refBuilder,
                     ACLEditorController *acl_editor_cont,
                     XAttrEditorController *xattr_editor_cont,
                     Application::EditMode mode)
    : Gtk::ApplicationWindow(cobject), m_refBuilder(refBuilder),
      acl_editor_controller(acl_editor_cont),
      xattr_editor_controller(xattr_editor_cont) {
  this->set_icon_name("eiciel");
  auto open_file = refBuilder->get_widget<Gtk::Button>("open-file");
  open_file->set_sensitive(true);
  open_file->signal_clicked().connect(
      [this]() { this->choose_file_to_open(); });

  auto open_directory = refBuilder->get_widget<Gtk::Button>("open-directory");
  open_directory->set_sensitive(true);
  open_directory->signal_clicked().connect(
      [this]() { this->choose_directory_to_open(); });

  ACLEditorWidget *acl_editor_widget =
      Gtk::Builder::get_widget_derived<ACLEditorWidget>(
          refBuilder, "acl-editor-widget", acl_editor_controller);
  (void)acl_editor_widget;

  XAttrEditorWidget* xattr_editor_widget = Gtk::Builder::get_widget_derived<XAttrEditorWidget>(
    refBuilder, "xattr-editor-widget", xattr_editor_controller);
  (void)xattr_editor_widget;

  filename_label = refBuilder->get_widget<Gtk::Label>("filename-label");

  Gtk::Stack* main_stack =
      refBuilder->get_widget<Gtk::Stack>("main-stack");
  Gtk::StackSwitcher* stack_switcher =
      refBuilder->get_widget<Gtk::StackSwitcher>("stack-switcher");

  if (mode != Application::EditMode::DEFAULT) {
    open_file->set_visible(false);
    open_directory->set_visible(false);
    stack_switcher->set_visible(false);

    Glib::ustring title = "Eiciel - ";
    if (mode == Application::EditMode::ACL) {
      title += _("Access Control List");
      main_stack->get_pages()->select_item(0, true);
    } else if (mode == Application::EditMode::XATTR) {
      title += _("Extended attributes");
      main_stack->get_pages()->select_item(1, true);
    }
    set_title(title);
  }

}

AppWindow::~AppWindow() {
  delete acl_editor_controller;
  delete xattr_editor_controller;
}

void AppWindow::choose_file_impl(const Glib::ustring &title,
                                 Gtk::FileChooser::Action action) {
  auto dialog = Gtk::FileChooserNative::create(title, *this, action);

  dialog->set_modal(true);
  dialog->set_transient_for(*this);

  dialog->signal_response().connect([dialog, this](int response_id) mutable {
    switch (response_id) {
    case Gtk::ResponseType::ACCEPT: {
      open_file(dialog->get_file());
      break;
    }
    default: {
      break;
    }
    }
    dialog.reset();
  });

  dialog->show();
}

void AppWindow::choose_file_to_open() {
  choose_file_impl(_("Select file"), Gtk::FileChooser::Action::OPEN);
}

void AppWindow::choose_directory_to_open() {
  choose_file_impl(_("Select directory"),
                   Gtk::FileChooser::Action::SELECT_FOLDER);
}

void AppWindow::open_file(const Glib::RefPtr<Gio::File> &file) {
  bool result = false;
  if (file->get_uri_scheme() == "file") {
    acl_editor_controller->open_file(file->get_path());
    result = acl_editor_controller->opened_file();
    xattr_editor_controller->open_file(file->get_path());
    result = result || acl_editor_controller->opened_file();
  }
  if (!result) {
    filename_label->set_text(_("No file opened"));
  } else {
    filename_label->set_text(file->get_path());
  }
}

} // namespace eiciel
