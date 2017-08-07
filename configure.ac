AC_PREREQ([2.63])
AC_INIT([Five or More], [3.25.90], [https://bugzilla.gnome.org/],
        [five-or-more], [https://wiki.gnome.org/Apps/Five%20or%20more])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-xz foreign])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE
AC_CONFIG_HEADERS([config.h])

AM_PROG_CC_C_O

GLIB_GSETTINGS

APPSTREAM_XML

dnl ###########################################################################
dnl Dependencies
dnl ###########################################################################

GLIB_REQUIRED=2.32
GTK_REQUIRED=3.12.0
RSVG_REQUIRED=2.32.0

PKG_CHECK_MODULES(FIVE_OR_MORE, [
  gmodule-2.0
  glib-2.0 >= $GLIB_REQUIRED
  gio-2.0 >= $GLIB_REQUIRED
  gtk+-3.0 >= $GTK_REQUIRED
  librsvg-2.0 >= $RSVG_REQUIRED
])

AC_PATH_PROG([DESKTOP_FILE_VALIDATE], [desktop-file-validate], [/bin/true])

dnl ###########################################################################
dnl Internationalization
dnl ###########################################################################

IT_PROG_INTLTOOL([0.50])
AC_SUBST(GETTEXT_PACKAGE, five-or-more)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [The gettext package name])

dnl ###########################################################################
dnl Documentation
dnl ###########################################################################

YELP_HELP_INIT

dnl ###########################################################################
dnl Files to generate
dnl ###########################################################################

AC_CONFIG_FILES([
Makefile
po/Makefile.in
data/Makefile
data/icons/Makefile
data/icons/hicolor/Makefile
help/Makefile
src/Makefile
])
AC_OUTPUT
