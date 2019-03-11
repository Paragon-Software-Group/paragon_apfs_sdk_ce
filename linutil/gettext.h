// <copyright file="gettext.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#if (HAVE_LIBINTL_H && ENABLE_NLS)
  #include <libintl.h>
  #define _(a) (gettext (a))
#else
  #define _(a) a
  #define textdomain(s)
  #define gettext(s) (s)
  #define dgettext(d,m) (m)
  #define dcgettext(d,m,t) (m)
  #define bindtextdomain(Domain,Directory)
#endif

#ifndef LOCALEDIR
# define LOCALEDIR "/usr/share/locale"
#endif
