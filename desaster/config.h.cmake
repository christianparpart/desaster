#ifndef desaster_config_h
#define desaster_config_h

#cmakedefine PACKAGE_NAME "@PACKAGE_NAME@"
#cmakedefine PACKAGE_VERSION "@PACKAGE_VERSION@"

#cmakedefine HAVE_SYS_RESOURCE_H
#cmakedefine HAVE_SYS_MMAN_H
#cmakedefine HAVE_SYS_LIMITS_H
#cmakedefine HAVE_PWD_H
#cmakedefine HAVE_SYSLOG_H
#cmakedefine HAVE_UUID_UUID_H
#cmakedefine HAVE_SYS_UTSNAME_H

#cmakedefine HAVE_FORK
#cmakedefine HAVE_CHROOT
#cmakedefine HAVE_PATHCONF
#cmakedefine HAVE_ACCEPT4
#cmakedefine HAVE_POSIX_FADVISE
#cmakedefine HAVE_READAHEAD

#endif
