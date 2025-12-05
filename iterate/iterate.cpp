#include <iostream>
#include <cstdio>

typedef unsigned char byte;

struct Header {
	int id;
	int curLen;
	int ofsNext;
	int crc;
	char name[16];
	char ver[16];
	char date[16];
	char reserved[64];
};

static inline int bswap32(int v)
{
	unsigned int x = (unsigned int)v;
	x = ((x & 0x000000FFu) << 24) |
		((x & 0x0000FF00u) << 8) |
		((x & 0x00FF0000u) >> 8) |
		((x & 0xFF000000u) >> 24);
	return (int)x;
}
//extern "C" {
//#include "LzmaDec.h"
//}
//static void *SzAlloc(void *, size_t size) { return malloc(size); }
//static void SzFree(void *, void *addr) { free(addr); }
//
//static ISzAlloc g_Alloc = { SzAlloc, SzFree };
//
//int lzma_decompress(byte* src, size_t srcLen, byte* dst, size_t dstLen)
//{
//	// LZMA header: 5-byte props + 8-byte uncompressed size
//	if (srcLen < LZMA_PROPS_SIZE) return -1;
//
//	Byte* props = src;
//	Byte* comp = src + LZMA_PROPS_SIZE;
//	SizeT compLen = srcLen - LZMA_PROPS_SIZE;
//
//	SizeT outLen = dstLen;
//	ELzmaStatus status;
//
//	SRes res = LzmaDecode(
//		dst, &outLen,
//		comp, &compLen,
//		props, LZMA_PROPS_SIZE,
//		LZMA_FINISH_ANY,
//		&status,
//		&g_Alloc
//	);
//
//	if (res != SZ_OK) return -1;
//	return (int)outLen;
//}
unsigned int mg_table_driven_crc(register unsigned int crc,
	register unsigned char *bufptr,
	register int len);

#define NO_CRC  0x4E435243

int main()
{
	FILE *f;
	f = fopen("dump.bin", "rb");
	if (f == 0) {
		f = fopen("W:/WaitingRoom/tuner202512/dump.bin", "rb");
	}
	if (!f) return 1;

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	byte *buf = (byte*)malloc(size);
	if (!buf) return 1;

	fread(buf, 1, size, f);
	fclose(f);

	byte *p = buf;
	const byte *end = buf + size;

	long offset = 0;
	int partIndex = 0;

	while (p + sizeof(Header) <= end) {
		Header *h = (Header*)p;

		int rawLen = h->ofsNext;
		h->ofsNext = bswap32(h->ofsNext);
		h->curLen = bswap32(h->curLen);

		std::cout
			<< "Partition " << partIndex
			<< " @ offset=" << offset
			<< " (0x" << std::hex << offset << std::dec << ")"
			<< " id=" << h->id
			<< " curLen=" << h->curLen
			<< " ofsNext=" << h->ofsNext
			<< " name=" << h->name
			<< " ver=" << h->ver
			<< " dat=" << h->date
			<< std::endl;

		if (h->ofsNext <= 0 || p + h->ofsNext > end) {
			std::cout << "Stopping: invalid length at offset " << offset << std::endl;
			break;
		}

		byte* data = p + sizeof(Header);
		int dataSize = h->ofsNext - sizeof(Header);

		if (h->crc == bswap32(NO_CRC)) {
			std::cout << "NCRC flag - skipping " << std::endl;
		} else {
			int check_crc = h->crc;
			int crc = mg_table_driven_crc(0xFFFFFFFF,
				p + 0x10, h->curLen);
			crc = bswap32(crc);
			if (check_crc != crc)
			{
				std::cout << "Bad crc " << std::endl;
			}
			else {
				std::cout << "Ok crc " << std::endl;
			}
		}


		{

			// Save raw block
			char outName[256];
			snprintf(outName, sizeof(outName),
				"part_%03d_%s_raw.bin", partIndex, h->name);

			FILE* outF = fopen(outName, "wb");
			if (outF) {
				fwrite(data, 1, dataSize, outF);
				fclose(outF);
				std::cout << "Saved raw as " << outName << std::endl;
			}
		}
		std::cout << "========================================== " << std::endl;


		partIndex++;
		p += h->ofsNext;
		offset += h->ofsNext;
	}

	long usedBytes = (long)(p - buf);
	long unused = size - usedBytes;

	if (unused > 0) {
		std::cout << "Unused bytes at end: " << unused
			<< " (0x" << std::hex << unused << std::dec << ")"
			<< std::endl;
	}
	else if (unused == 0) {
		std::cout << "No unused bytes at end." << std::endl;
	}
	else {
		std::cout << "Error: parser went past buffer end (unexpected!)" << std::endl;
	}

	free(buf);
	return 0;
}
