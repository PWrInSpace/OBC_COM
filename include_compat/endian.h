#ifndef _COMPAT_ENDIAN_H
#define _COMPAT_ENDIAN_H

#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN    4321
#define __BYTE_ORDER    __LITTLE_ENDIAN

#define le16toh(x) (x)
#define htole16(x) (x)
#define be16toh(x) __builtin_bswap16(x)
#define htobe16(x) __builtin_bswap16(x)

#define le32toh(x) (x)
#define htole32(x) (x)
#define be32toh(x) __builtin_bswap32(x)
#define htobe32(x) __builtin_bswap32(x)

#define le64toh(x) (x)
#define htole64(x) (x)
#define be64toh(x) __builtin_bswap64(x)
#define htobe64(x) __builtin_bswap64(x)

#endif
