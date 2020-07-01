#include "bitmap_image.hpp"
#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <ctime>

using namespace std;
const string ErrorMessage = "Wrong arguments. Enter BitmapBlur.exe <Input Image> <Output Image> <Threads Number> <Core Number>";

struct ThreadData
{
	bitmap_image inImage;
	bitmap_image* outImage;
	int threadsNumber;
	int coreNumber;
	int index;
	int startBlur;
	int endBlur;
};

vector<rgb_t> GetPixels(const int width, const int height, bitmap_image& inImage)
{
	vector<rgb_t> pixels;
	for (int i = width - 5; i <= width + 5; i++)
	{
		for (int j = height - 5; j <= height + 5; j++)
		{
			if (i >= 0 && j >= 0 && i < (int)inImage.width() && j < (int)inImage.height())
			{
				rgb_t pixel = inImage.get_pixel(i, j);
				pixels.push_back(pixel);
			}
		}
	}
	return pixels;
}

void Blur(ThreadData* thread, int imageWidth)
{
	for (int i = 0; i < imageWidth; i++)
	{
		for (int j = thread->startBlur; j < thread->endBlur; j++)
		{
			vector<rgb_t> pixels = GetPixels(i, j, thread->inImage);

			int Reds = 0;
			int Greens = 0;
			int Blues = 0;

			for (const auto& pixel : pixels)
			{
				Reds += pixel.red;
				Greens += pixel.green;
				Blues += pixel.blue;
			}

			thread->outImage->set_pixel(i, j,
				(unsigned char)(Reds / pixels.size()),
				(unsigned char)(Greens / pixels.size()),
				(unsigned char)(Blues / pixels.size())
			);
		}
	}
}

DWORD WINAPI ThreadProc(CONST LPVOID lpParam)
{
	ThreadData* thread = static_cast<ThreadData*>(lpParam);
	const int inImageWidth = thread->inImage.width();
	Blur(thread, inImageWidth);
	ExitThread(0);
}

void BlurThread(const string input, const string output, const int threadsNumber, const int coreNumber)
{
	bitmap_image inImage(input);
	bitmap_image outImage(inImage);

	vector<HANDLE> handles(threadsNumber);
	const int affinityMask = (1 << coreNumber) - 1;

	const int inImageHeight = inImage.height();
	const int heightForThread = inImageHeight / threadsNumber;

	vector<ThreadData> thread;

	for (int i = 0; i < threadsNumber; i++)
	{
		const int startBlur = heightForThread * i;
		int endBlur;

		if (i + 1 == threadsNumber)
		{
			endBlur = inImageHeight;
		}
		else
		{
			endBlur = heightForThread * (i + 1);
		}

		ThreadData threadData = {
			inImage,
			&outImage,
			threadsNumber,
			coreNumber,
			i,
			startBlur,
			endBlur,
		};

		thread.push_back(threadData);
	}

	for (int i = 0; i < threadsNumber; i++)
	{
		handles[i] = CreateThread(NULL, 0, &ThreadProc, &thread[i], CREATE_SUSPENDED, NULL);
		SetThreadAffinityMask(handles[i], affinityMask);
	}

	for (const auto& handle : handles)
	{
		ResumeThread(handle);
	}

	WaitForMultipleObjects((DWORD)handles.size(), handles.data(), true, INFINITE);
	outImage.save_image(output);
}


int main(int argc, char* argv[])
{

	if (argc < 5)
	{
		cout << ErrorMessage << endl;
		return 0;
	}


	try
	{
		clock_t start = clock();
		int threadsNumber;
		int coreNumber;
		const char* inImage;
		const char* outImage;
		inImage = argv[1];
		outImage = argv[2];
		threadsNumber = stoi(argv[3]);
		coreNumber = stoi(argv[4]);

		BlurThread(inImage, outImage, threadsNumber, coreNumber);

		clock_t end = clock();
		cout << "Time: " << difftime(end, start) << " ms" << endl;
	}
	catch (exception e)
	{
		cout << e.what() << endl;
	}
}