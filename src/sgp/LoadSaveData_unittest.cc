// -*-coding: utf-8-unix;-*-

#include "gtest/gtest.h"

#include "LoadSaveData.h"

#include "externalized/RustInterface.h"
#include "externalized/TestUtils.h"


TEST(LoadSaveData, integers)
{
	char buf[100];

	{
		DataWriter writer(buf);
		writer.writeU8(0x01);
		writer.writeU8(0x00);
		writer.writeU8(0xff);

		DataReader reader(buf);
		EXPECT_EQ(reader.readU8(), 0x01);
		EXPECT_EQ(reader.readU8(), 0x00);
		EXPECT_EQ(reader.readU8(), 0xff);

		EXPECT_EQ(reader.getConsumed(), 3u);
		EXPECT_EQ(writer.getConsumed(), 3u);
	}

	{
		DataWriter writer(buf);
		writer.writeU16(0x0123);
		writer.writeU16(0x0000);
		writer.writeU16(0xffff);

		DataReader reader(buf);
		EXPECT_EQ(reader.readU16(), 0x0123);
		EXPECT_EQ(reader.readU16(), 0x0000);
		EXPECT_EQ(reader.readU16(), 0xffff);

		EXPECT_EQ(reader.getConsumed(), 6u);
		EXPECT_EQ(writer.getConsumed(), 6u);
	}

	{
		DataWriter writer(buf);
		writer.writeU32(0x01234567);
		writer.writeU32(0x00000000);
		writer.writeU32(0xffffffff);

		DataReader reader(buf);
		EXPECT_EQ(reader.readU32(), 0x01234567u);
		EXPECT_EQ(reader.readU32(), 0x00000000u);
		EXPECT_EQ(reader.readU32(), 0xffffffffu);

		EXPECT_EQ(reader.getConsumed(), 12u);
		EXPECT_EQ(writer.getConsumed(), 12u);
	}
}

TEST(LoadSaveData, wcharToUTF16English)
{
	char buf[100];

	DataWriter writer(buf);
	writer.writeStringAsUTF16(L"test", 5);

	// read as 5 uint16
	{
		DataReader reader(buf);
		EXPECT_EQ(reader.readU16(), 't');
		EXPECT_EQ(reader.readU16(), 'e');
		EXPECT_EQ(reader.readU16(), 's');
		EXPECT_EQ(reader.readU16(), 't');
		EXPECT_EQ(reader.readU16(), 0x0000);

		EXPECT_EQ(reader.getConsumed(), 10u);
		EXPECT_EQ(writer.getConsumed(), 10u);
	}
}

TEST(LoadSaveData, wcharToUTF16Russian)
{
	char buf[100];

	DataWriter writer(buf);
	writer.writeStringAsUTF16(L"тест", 5);

	// read as 5 uint16
	{
		DataReader reader(buf);
		EXPECT_EQ(reader.readU16(), 0x0442);
		EXPECT_EQ(reader.readU16(), 0x0435);
		EXPECT_EQ(reader.readU16(), 0x0441);
		EXPECT_EQ(reader.readU16(), 0x0442);
		EXPECT_EQ(reader.readU16(), 0x000);
	}
}

TEST(LoadSaveData, utf16ToWideString)
{
	char buf[100];
	DataWriter writer(buf);

	writer.writeU16(0x0442);
	writer.writeU16(0x0435);
	writer.writeU16(0x0441);
	writer.writeU16(0x0442);
	writer.writeU16(0x000);

	{
		DataReader reader(buf);
		EXPECT_EQ(reader.readUTF16(5), "тест");

		EXPECT_EQ(reader.getConsumed(), 10u);
		EXPECT_EQ(writer.getConsumed(), 10u);
	}

	{
		DataReader reader(buf);
		wchar_t wideBuf[10];
		reader.readUTF16(wideBuf, 5);
		EXPECT_STREQ(wideBuf, L"тест");

		EXPECT_EQ(reader.getConsumed(), 10u);
		EXPECT_EQ(writer.getConsumed(), 10u);
	}
}

TEST(LoadSaveData, utf32ToWideString)
{
	char buf[100];
	DataWriter writer(buf);

	writer.writeU32(0x00000442);
	writer.writeU32(0x00000435);
	writer.writeU32(0x00000441);
	writer.writeU32(0x00000442);
	writer.writeU32(0x00000000);

	{
		DataReader reader(buf);
		EXPECT_EQ(reader.readUTF32(5), "тест");

		EXPECT_EQ(reader.getConsumed(), 20u);
		EXPECT_EQ(writer.getConsumed(), 20u);
	}

	{
		DataReader reader(buf);
		wchar_t wideBuf[10];
		reader.readUTF32(wideBuf, 5);
		EXPECT_STREQ(wideBuf, L"тест");

		EXPECT_EQ(reader.getConsumed(), 20u);
		EXPECT_EQ(writer.getConsumed(), 20u);
	}
}

TEST(LoadSaveData, floatAndDoubleFormat)
{
	// We need to make sure that on the current platform
	// float and double have the same binary representation
	// as on Windows.  Otherwise, there will be problems with
	// loading saved games made on Windows.

	std::string floatsPath = FileMan::joinPaths(GetExtraDataDir(), "unittests/datatypes/floats.bin");
	std::string doublesPath = FileMan::joinPaths(GetExtraDataDir(), "unittests/datatypes/doubles.bin");

	// // Test data were previously written with the following code.
	// {
	//   float f;
	//   FILE *floats = fopen(floatsPath, "wb");
	//   f = 0;              fwrite(&f, sizeof(float), 1, floats);
	//   f = 1;              fwrite(&f, sizeof(float), 1, floats);
	//   f = -1;             fwrite(&f, sizeof(float), 1, floats);
	//   f = 1.1234678f;     fwrite(&f, sizeof(float), 1, floats);
	//   f = 12345.678f;     fwrite(&f, sizeof(float), 1, floats);
	//   fclose(floats);

	//   double d;
	//   FILE *doubles = fopen(doublesPath, "wb");
	//   d = 0;              fwrite(&d, sizeof(double), 1, doubles);
	//   d = 1;              fwrite(&d, sizeof(double), 1, doubles);
	//   d = -1;             fwrite(&d, sizeof(double), 1, doubles);
	//   d = 1.1234678;      fwrite(&d, sizeof(double), 1, doubles);
	//   d = 12345.678;      fwrite(&d, sizeof(double), 1, doubles);
	//   fclose(doubles);

	//   printf("float size: %d\n", sizeof(float));
	//   printf("double size: %d\n", sizeof(double));
	// }

	ASSERT_EQ(sizeof(float),  4u);
	ASSERT_EQ(sizeof(double), 8u);

	{
		RustPointer<VecU8> buf(Fs_read(floatsPath.c_str()));
		ASSERT_TRUE(buf);
		ASSERT_EQ(VecU8_len(buf.get()), sizeof(float) * 5);
		float f;

		const uint8_t* S = VecU8_as_ptr(buf.get());
		EXTR_FLOAT(S, f); EXPECT_EQ(f, 0         );
		EXTR_FLOAT(S, f); EXPECT_EQ(f, 1         );
		EXTR_FLOAT(S, f); EXPECT_EQ(f, -1        );
		EXTR_FLOAT(S, f); EXPECT_EQ(f, 1.1234678f);
		EXTR_FLOAT(S, f); EXPECT_EQ(f, 12345.678f);
	}

	{
		RustPointer<VecU8> buf(Fs_read(doublesPath.c_str()));
		ASSERT_TRUE(buf);
		ASSERT_EQ(VecU8_len(buf.get()), sizeof(double) * 5);
		double d;

		const uint8_t* S = VecU8_as_ptr(buf.get());
		EXTR_DOUBLE(S, d); EXPECT_EQ(d, 0         );
		EXTR_DOUBLE(S, d); EXPECT_EQ(d, 1         );
		EXTR_DOUBLE(S, d); EXPECT_EQ(d, -1        );
		EXTR_DOUBLE(S, d); EXPECT_EQ(d, 1.1234678 );
		EXTR_DOUBLE(S, d); EXPECT_EQ(d, 12345.678 );
	}
}
