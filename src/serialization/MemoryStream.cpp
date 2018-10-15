#include "MemoryStream.h"
#include <algorithm> // std::max

void OutputMemoryStream::Write(const void *inData, size_t inByteCount)
{
	// make sure we have space
	const uint32_t resultHead = mHead + static_cast<uint32_t>(inByteCount);
	if (resultHead > mCapacity)
	{
		ReallocBuffer(std::max(mCapacity * 2, resultHead));
	}

	// Copy into buffer at head
	std::memcpy(mBuffer + mHead, inData, inByteCount);

	// Increment head for next write
	mHead = resultHead;
}

void OutputMemoryStream::ReallocBuffer(uint32_t inNewLength)
{
	mBuffer = static_cast<char*>(std::realloc(mBuffer, inNewLength));
	// TODO: handle realloc failure
	mCapacity = inNewLength;
}

void InputMemoryStream::Read(void *outData, size_t inByteCount) const
{
	uint32_t resultHead = mHead + static_cast<uint32_t>(inByteCount);
	if (resultHead > mCapacity)
	{
		// TODO: handle error, no data to read
	}

	std::memcpy(outData, mBuffer + mHead, inByteCount);
	mHead = resultHead;
}
