
#pragma once

template<int N>
struct StringBuilder
{
public:
    StringBuilder() :
        mLength(0)
    {
        mString[0] = '\0';
    }

    void Append(char c)
    {
        int length = mLength;
        char* thisStr = mString;

        thisStr[length++] = c;
        thisStr[length] = '\0';

        mLength = length;
    }

    void Append(const char* str)
    {
        int length = mLength;
        char* thisStr = mString;

        int index = 0;
        while ((length < (N - 1)) && (str[index] != '\0'))
        {
            thisStr[length++] = str[index++];
        }
        thisStr[length] = '\0';
        mLength = length;
    }

    void Reduce(int numChars)
    {
        int newLength = mLength - numChars;
        if (newLength < 0)
        {
            newLength = 0;
        }
        mLength = newLength;
        mString[newLength] = '\0';
    }

    const char* c_str() const
    {
        return mString;
    }

    char last() const
    {
        return (mLength > 0) ? mString[mLength-1] : '\0';
    }

private:
    int mLength;
    char mString[N];
};
