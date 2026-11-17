static const unsigned char SafePool[] = {
	0,1,2,3,4,5,6,7,
	16,17,18,19,20,21,22,23,
	// optionally add inverse, etc.
}

#define SAFE_POOL_SIZE (sizeof(SafePool))

// to use:
// rg = SafePool[qrandomJ(peek(0x276)) % SAFE_POOL_SIZE;

