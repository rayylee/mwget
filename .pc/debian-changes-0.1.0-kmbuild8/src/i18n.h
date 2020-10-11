/*  MWget - A Multi download for all POSIX systems.
 *  Homepage: http://mwget.sf.net
 *  Copyright (C) 2005- rgwan,xiaosuo
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#undef _ //草，就是被这里卡了很久。一直不知道_被define在哪里。现在直接undef掉，然后重新def
#  define _(String) gettext (String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif
//#define DEBUG_LOCALE 1 //测试locale用
void seti18npackage(void)
{
#ifdef ENABLE_NLS
#ifdef DEBUG_LOCALE
printf("DEBUG:I18N Enabled.\n");
#endif
/* Set the current locale.  */
setlocale (LC_ALL, "");
bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
textdomain(GETTEXT_PACKAGE);
#ifdef DEBUG_LOCALE
printf("GETTEXT_PACKAGE:%s LOCALEDIR:%s\n",GETTEXT_PACKAGE, LOCALEDIR);
printf("TEXTDOMAIN:%s\n",GETTEXT_PACKAGE);
printf("DEBUG:TRY _(\"missing URL\")\n");
printf("%s\n",_("missing URL"));
#endif
#endif /* ENABLE_NLS */
}
