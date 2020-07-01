#include "bitmap_image.hpp"
#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <ctime>

using namespace std;


struct ThreadData
{
	bitmap_image inImage;
	bitmap_image* outImage;
	int threadsNumber;
	int coreNumber;
	int index;
	int startBlur;
	int endBlur;
	clock_t startTime;
	ofstream* outputStream;

};

double GetTimeDifference(const clock_t& start)
{
	clock_t end = clock();
	return difftime(end, start);
}

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

enum ThreadPriority
{
	BELOW = -1,
	NORMAL = 0,
	ABOVE = 1
};

void Blur(ThreadData* thread, int imageWidth, ofstream* outstream)
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

			thread->outImage->set_pixel(
				i,
				j,
				(unsigned char)(Reds / pixels.size()),
				(unsigned char)(Greens / pixels.size()),
				(unsigned char)(Blues / pixels.size())
			);

			*thread->outputStream << thread->index + 1 << "\t" << GetTimeDifference(thread->startTime) << endl;

		}

	}
}

DWORD WINAPI ThreadProc(CONST LPVOID lpParam)
{
	ThreadData* thread = static_cast<ThreadData*>(lpParam);

	const int startBlur = thread->startBlur;
	const int endBlur = thread->endBlur;
	const int inImageWidth = thread->inImage.width();
	bitmap_image inputImage = thread->inImage;
	bitmap_image* outputImage = thread->outImage;

	ofstream* outputStream = thread->outputStream;


	Blur(thread, inImageWidth, outputStream);


	ExitThread(0);
}

void BlurThread(const string input, const string output, const int threadsNumber, const int coreNumber, const vector<ThreadPriority> threadPriorities)
{
	clock_t startTime = clock();
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
			startTime,
			new ofstream("thread" + to_string(i + 1) + ".txt"),
		};

		thread.push_back(threadData);
	}

	for (int i = 0; i < threadsNumber; i++)
	{
		handles[i] = CreateThread(NULL, 0, &ThreadProc, &thread[i], CREATE_SUSPENDED, NULL);
		SetThreadAffinityMask(handles[i], affinityMask);
		SetThreadPriority(handles[i], threadPriorities[i]);
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
	string firstArg(argv[1]);
	if (firstArg == "/?")
	{
		cout << "For Example" << endl;
		cout << "BitmapBlur.exe <Input Image> <Output Image> <Threads Number> <Core Number>" << endl;
		cout << "Then set priorities : " << "below_normal, normal, above_normal or 1 (below normal), 2 (normal), 3 (above_normal)" << endl;
		cout << "The number of thread priorities must be equal to the number of threads" << endl;
		return 0;
	
	}
	else
	{
		if (argc < 5)
		{
			cout << "Wrong arguments. Help: BitmapBlur.exe /?" << endl;
			return 0;
		}
		clock_t start = clock();
		int threadsNumber;
		int coreNumber;
		const char* inImage;
		const char* outImage;
		inImage = argv[1];
		outImage = argv[2];
		threadsNumber = stoi(argv[3]);
		coreNumber = stoi(argv[4]);


		vector<ThreadPriority> priorities;

		for (size_t i = 0; i < threadsNumber; i++)
		{
			cout << "Priority for thread num " << i + 1 << ": ";
			string str;
			cin >> str;
			bool flag = true;
			do {
				if ((str == "below_normal") || (atoi(str.c_str()) == 1))
				{
					priorities.push_back(ThreadPriority::BELOW);
					flag = false;
				}
				else if ((str == "normal") || (atoi(str.c_str()) == 2))
				{
					priorities.push_back(ThreadPriority::NORMAL);
					flag = false;
				}
				else if ((str == "above_normal") || (atoi(str.c_str()) == 3))
				{
					priorities.push_back(ThreadPriority::ABOVE);
					flag = false;
				}
				else
				{
					cout << "Priorities for threads:\n"
						<< "\t 1. below_normal\n"
						<< "\t 2. normal\n"
						<< "\t 3. above_normal\n"
						<< "Input example: 1\n"
						<< "Or: below_normal\n"
						<< endl;
					return 0;
				}
			} while (flag);


		}


		BlurThread(inImage, outImage, threadsNumber, coreNumber, priorities);

		clock_t end = clock();
		cout << "Time: " << difftime(end, start) << " ms" << endl;

	return 0;
	}

}



