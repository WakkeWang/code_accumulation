#ifndef __LINUX_KERNEL_H
#define __LINUX_KERNEL_H

#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))
#define ALIGN(x, a)		((x + (a - 1)) & ~(a - 1))

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:    the pointer to the member.
 * @type:   the type of the container struct this is embedded in.
 * @member: the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({          \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})


#endif
