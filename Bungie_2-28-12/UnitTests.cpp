#include "common.h"
#include "string.h"
#include <exception>

using namespace System;
using namespace System::IO;
using namespace System::Text;
using namespace System::Collections::Generic;
using namespace System::Reflection;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace Runtime::InteropServices;

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

        String ^ GetAssemblyPath () {
            Assembly ^ executingAssembly = Assembly::GetExecutingAssembly();
            Uri ^ uri = gcnew Uri(executingAssembly->CodeBase);

            String ^ result = Uri::UnescapeDataString(uri->AbsolutePath);
            if (String::IsNullOrWhiteSpace(result))
                result = executingAssembly->Location;

            return result;
        }

        //
        // reverse_words tests
        //
        
		[TestMethod]
		void ReverseWordsDoesNotOverOrUnderrunBuffer() {
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
		void ReverseWordsReversesWords() {
			char buffer[256]   = "Now is the winter of our discontent made glorious summer by this son of York";
            char expected[256] = "York of son this by summer glorious made discontent our of winter the is Now";

            reverse_words(buffer);

            Assert::AreEqual(
                gcnew String(expected), 
                gcnew String(buffer)
            );
		};

		[TestMethod]
		void ReverseWordsDoesNotDestroyNullTerminator() {
			char buffer[256]       = "The quick brown fox jumped over the lazy dogs\0abcd\0efgh";
            size_t expected_length = strlen(buffer);

            reverse_words(buffer);

            Assert::AreEqual(expected_length, strlen(buffer));
		};

		[TestMethod]
		void ReverseWordsWorksOnEmptyString() {
			char buffer[256]       = "\0abcd\0efgh";
            size_t expected_length = strlen(buffer);

            reverse_words(buffer);

            Assert::AreEqual(expected_length, strlen(buffer));
		};

		[TestMethod]
		void ReverseWordsWorksOnSingleCharacter() {
			char buffer[256]       = "a\0abcd\0efgh";
            size_t expected_length = strlen(buffer);

            reverse_words(buffer);

            Assert::AreEqual(expected_length, strlen(buffer));
            Assert::AreEqual(
                gcnew String("a"),
                gcnew String(buffer)
            );
		};

		[TestMethod]
		void ReverseWordsWorksOnSingleWord() {
			char buffer[256]       = "abc\0abcd\0efgh";
            size_t expected_length = strlen(buffer);

            reverse_words(buffer);

            Assert::AreEqual(expected_length, strlen(buffer));
            Assert::AreEqual(
                gcnew String("abc"),
                gcnew String(buffer)
            );
		};

		[TestMethod]
		void ReverseWordsWorksOnWordsWithEvenCharacterCount() {
			char buffer[256]       = "abcd\0abcd\0efgh";
            size_t expected_length = strlen(buffer);

            reverse_words(buffer);

            Assert::AreEqual(expected_length, strlen(buffer));
            Assert::AreEqual(
                gcnew String("abcd"),
                gcnew String(buffer)
            );
		};

		[TestMethod]
		void ReverseWordsThrowsIfAWordIsTooLong() {
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

        template <typename T>
        void AssertPointersEqual(T* expected, T* actual) {
            Assert::AreEqual((unsigned long long)expected, (unsigned long long)actual);
        }

        void MakeNode(struct s_node out[], unsigned index, int nextIndex, int referenceIndex) {
            out[index].next = (nextIndex >= 0) ? &out[nextIndex] : 0;
            out[index].reference = (referenceIndex >= 0) ? &out[referenceIndex] : 0;
        }

        [TestMethod]
        void DuplicatesList() {
            struct s_node sourceList[4];
            struct s_node * duplicateNodes[4];
            struct s_node * zero = 0;
            struct s_node * head = &sourceList[0];

            MakeNode(sourceList, 0,  1,  0);
            MakeNode(sourceList, 1,  2,  3);
            MakeNode(sourceList, 2,  3,  3);
            MakeNode(sourceList, 3, -1,  1);

            struct s_node * duplicateHead = duplicate_list(head);
            Assert::AreEqual(4U, copy_list_to_array(duplicateHead, duplicateNodes, 4));

            AssertPointersEqual(duplicateNodes[1], duplicateNodes[0]->next);
            AssertPointersEqual(duplicateNodes[2], duplicateNodes[1]->next);
            AssertPointersEqual(duplicateNodes[3], duplicateNodes[2]->next);
            AssertPointersEqual(zero, duplicateNodes[3]->next);

            AssertPointersEqual(duplicateNodes[0], duplicateNodes[0]->reference);
            AssertPointersEqual(duplicateNodes[3], duplicateNodes[1]->reference);
            AssertPointersEqual(duplicateNodes[3], duplicateNodes[2]->reference);
            AssertPointersEqual(duplicateNodes[1], duplicateNodes[3]->reference);

            free_list(duplicateHead);
        }

        [TestMethod]
        void ThrowsIfListContainsCycle() {
            struct s_node sourceList[4];
            struct s_node * head = &sourceList[0];

            MakeNode(sourceList, 0,  1,  0);
            MakeNode(sourceList, 1,  2,  0);
            MakeNode(sourceList, 2,  3,  0);
            MakeNode(sourceList, 3,  0,  0);

            try {
                struct s_node * duplicateHead = duplicate_list(head);
                free_list(duplicateHead);
                Assert::Fail("Should have thrown a C++ exception");
            } catch (std::exception exc) {
            }
        }

        //
        // boggle solver tests
        // 

        [TestMethod]
        void LoadsDictionary() {
            String ^ assemblyDir = Path::GetDirectoryName(GetAssemblyPath());
            String ^ dictionaryPath = Path::Combine(assemblyDir, gcnew String("..\\tinydictionary.txt"));

            const char * dictionaryPathPtr = (const char *)(Marshal::StringToHGlobalAnsi(dictionaryPath)).ToPointer();
            Boggle::Dictionary * dictionary = new Boggle::Dictionary(dictionaryPathPtr);
            Marshal::FreeHGlobal(IntPtr((void*)dictionaryPathPtr));

            Assert::AreEqual(7U, dictionary->wordCount);

            delete dictionary;
        }

        [TestMethod]
        void LoadsBigDictionary() {
            String ^ assemblyDir = Path::GetDirectoryName(GetAssemblyPath());
            String ^ dictionaryPath = Path::Combine(assemblyDir, gcnew String("..\\enable1.txt"));

            const char * dictionaryPathPtr = (const char *)(Marshal::StringToHGlobalAnsi(dictionaryPath)).ToPointer();
            Boggle::Dictionary * dictionary = new Boggle::Dictionary(dictionaryPathPtr);
            Marshal::FreeHGlobal(IntPtr((void*)dictionaryPathPtr));

            Assert::AreEqual(172820U, dictionary->wordCount);

            delete dictionary;
        }

        [TestMethod]
        void LoadsSmallBoard() {
            String ^ assemblyDir = Path::GetDirectoryName(GetAssemblyPath());
            String ^ boardPath = Path::Combine(assemblyDir, gcnew String("..\\smallboard.txt"));

            const char * boardPathPtr = (const char *)(Marshal::StringToHGlobalAnsi(boardPath)).ToPointer();
            Boggle::Board * board = Boggle::Board::fromFile(boardPathPtr);
            Marshal::FreeHGlobal(IntPtr((void*)boardPathPtr));

            Assert::AreEqual(3U, board->width);
            Assert::AreEqual(3U, board->height);

            delete board;
        }
    };
}
