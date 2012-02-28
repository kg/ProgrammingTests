#include "common.h"
#include "string.h"
#include <exception>

using namespace System;
using namespace System::Text;
using namespace System::Collections::Generic;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace Test
{
	[TestClass]
	public ref class UnitTests
	{
	private:
		TestContext^ testContextInstance;

	public: 
		property Microsoft::VisualStudio::TestTools::UnitTesting::TestContext^ TestContext
		{
			Microsoft::VisualStudio::TestTools::UnitTesting::TestContext^ get()
			{
				return testContextInstance;
			}
			System::Void set(Microsoft::VisualStudio::TestTools::UnitTesting::TestContext^ value)
			{
				testContextInstance = value;
			}
		};

        //
        // reverse_words tests
        //
        
		[TestMethod]
		void ReverseWordsDoesNotOverOrUnderrunBuffer()
		{
			char buffer[256];
            buffer[0]   = ' ';
            buffer[1]   = ' ';
            buffer[253] = ' ';
            buffer[254] = '\0';
            buffer[255] = '\0';

            reverse_words(buffer + 2);

            Assert::AreEqual(buffer[0],   ' ');
            Assert::AreEqual(buffer[1],   ' ');
            Assert::AreEqual(buffer[253], ' ');
            Assert::AreEqual(buffer[254], '\0');
            Assert::AreEqual(buffer[255], '\0');
		};

		[TestMethod]
		void ReverseWordsReversesWords()
		{
			char buffer[256]   = "Now is the winter of our discontent made glorious summer by this son of York";
            char expected[256] = "York of son this by summer glorious made discontent our of winter the is Now";

            reverse_words(buffer);

            Assert::AreEqual(
                gcnew System::String(expected), 
                gcnew System::String(buffer)
            );
		};

		[TestMethod]
		void ReverseWordsDoesNotDestroyNullTerminator()
		{
			char buffer[256]       = "The quick brown fox jumped over the lazy dogs\0abcd\0efgh";
            size_t expected_length = strlen(buffer);

            reverse_words(buffer);

            Assert::AreEqual(expected_length, strlen(buffer));
		};

		[TestMethod]
		void ReverseWordsWorksOnEmptyString()
		{
			char buffer[256]       = "\0abcd\0efgh";
            size_t expected_length = strlen(buffer);

            reverse_words(buffer);

            Assert::AreEqual(expected_length, strlen(buffer));
		};

		[TestMethod]
		void ReverseWordsWorksOnSingleCharacter()
		{
			char buffer[256]       = "a\0abcd\0efgh";
            size_t expected_length = strlen(buffer);

            reverse_words(buffer);

            Assert::AreEqual(expected_length, strlen(buffer));
            Assert::AreEqual(
                gcnew System::String("a"),
                gcnew System::String(buffer)
            );
		};

		[TestMethod]
		void ReverseWordsWorksOnSingleWord()
		{
			char buffer[256]       = "abc\0abcd\0efgh";
            size_t expected_length = strlen(buffer);

            reverse_words(buffer);

            Assert::AreEqual(expected_length, strlen(buffer));
            Assert::AreEqual(
                gcnew System::String("abc"),
                gcnew System::String(buffer)
            );
		};

		[TestMethod]
		void ReverseWordsWorksOnWordsWithEvenCharacterCount()
		{
			char buffer[256]       = "abcd\0abcd\0efgh";
            size_t expected_length = strlen(buffer);

            reverse_words(buffer);

            Assert::AreEqual(expected_length, strlen(buffer));
            Assert::AreEqual(
                gcnew System::String("abcd"),
                gcnew System::String(buffer)
            );
		};

		[TestMethod]
		void ReverseWordsThrowsIfAWordIsTooLong()
		{
			char buffer[1024] = "word1 abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz word3 word4";

            try {
                reverse_words(buffer);
                Assert::Fail("Should have thrown a C++ exception");
            } catch (std::exception exc) {
            }
		};

        //
        // duplicate_list tests
        //
    };
}
