#include <stdio.h>
#include "log/log.h"
#include "xptools/UART.h"
#include <list>
#include <map>

using namespace std;

void ReadFrame(UART& uart);
uint16_t ReadUint16(UART& uart);

class SampleData
{
public:

	SampleData()
		: count_0p3(0)
		, count_0p5(0)
		, count_1p0(0)
		, count_2p5(0)
		, count_5p0(0)
		, count_10p0(0)
	{
	}

	uint64_t count_0p3;
	uint64_t count_0p5;
	uint64_t count_1p0;
	uint64_t count_2p5;
	uint64_t count_5p0;
	uint64_t count_10p0;

	bool operator!=(const SampleData& rhs)
	{
		return
			(count_0p3 != rhs.count_0p3) ||
			(count_0p5 != rhs.count_0p5) ||
			(count_1p0 != rhs.count_1p0) ||
			(count_2p5 != rhs.count_2p5) ||
			(count_5p0 != rhs.count_5p0) ||
			(count_10p0 != rhs.count_10p0);
	}

	SampleData& operator+=(const SampleData& rhs)
	{
		count_0p3 += rhs.count_0p3;
		count_0p5 += rhs.count_0p5;
		count_1p0 += rhs.count_1p0;
		count_2p5 += rhs.count_2p5;
		count_5p0 += rhs.count_5p0;
		count_10p0 += rhs.count_10p0;
		return *this;
	}
};

list<SampleData> g_counts;

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		printf("Usage: pms5003 /dev/ttyUSB0\n");
		return 0;
	}

	g_log_sinks.emplace(g_log_sinks.begin(), new ColoredSTDLogSink(Severity::DEBUG));

	UART uart(argv[1], 9600);
	fflush(stdout);

	const int max_samples = 500;

	uint8_t c;
	while(true)
	{
		//There's no real framing, so just throw away data until we get a valid frame start
		uart.Read(&c, 1);
		if(c != 0x42)
			continue;
		uart.Read(&c, 1);
		if(c != 0x4d)
			continue;
		ReadFrame(uart);

		//Remove old stuff
		while(g_counts.size() > max_samples)
			g_counts.pop_front();

		//Calculate total sample volume
		size_t count = g_counts.size();
		int64_t ml = count * 100;
		double m3 = ml * 1e-6;
		LogNotice("Integrated totals (from %.1f L / %.2f m^3)\n", ml*1e-3, m3);
		LogIndenter li;

		//Get total particles
		SampleData sum;
		for(auto s : g_counts)
			sum += s;

		//Convert to float
		const int sizes[6] = {300, 500, 1000, 2500, 5000, 10000};
		uint64_t counts[6] =
		{
			sum.count_0p3,
			sum.count_0p5,
			sum.count_1p0,
			sum.count_2p5,
			sum.count_5p0,
			sum.count_10p0
		};
		double particles_per_m3[6] =
		{
			sum.count_0p3 / m3,
			sum.count_0p5 / m3,
			sum.count_1p0 / m3,
			sum.count_2p5 / m3,
			sum.count_5p0 / m3,
			sum.count_10p0 / m3
		};

		for(size_t i=0; i<6; i++)
			LogNotice("%4.1f um: %8zu (%10.0f / m^3)\n", sizes[i]*1e-3, counts[i], particles_per_m3[i]);

		//Dump the latest readings into /tmp
		FILE* fp = fopen("/tmp/particle_count", "w");
		fprintf(fp, "%.0f,%.0f,%.0f,%.0f,%.0f,%.0f\n",
			particles_per_m3[0], particles_per_m3[1], particles_per_m3[2],
			particles_per_m3[3], particles_per_m3[4], particles_per_m3[5]);
		fclose(fp);
	}

	return 0;
}

void ReadFrame(UART& uart)
{
	//Read length (expecting 28)
	auto len = ReadUint16(uart);
	if(len != 28)
		return;

	//PM1.0, PM2.5, PM10 concentration levels using standard particles
	/* uint16_t pm1_std = */ ReadUint16(uart);
	/* uint16_t pm25_std = */ ReadUint16(uart);
	/* uint16_t pm10_std = */ ReadUint16(uart);

	//PM1.0, PM2.5, PM10 concentration levels using atmospheric environment
	/* uint16_t pm1_std = */ ReadUint16(uart);
	/* uint16_t pm25_std = */ ReadUint16(uart);
	/* uint16_t pm10_std = */ ReadUint16(uart);

	//List of particles by size per 100ml of air
	SampleData sample;
	sample.count_0p3 = ReadUint16(uart);
	sample.count_0p5 = ReadUint16(uart);
	sample.count_1p0 = ReadUint16(uart);
	sample.count_2p5 = ReadUint16(uart);
	sample.count_5p0 = ReadUint16(uart);
	sample.count_10p0 = ReadUint16(uart);

	//The sensor likes to repeat sample values but they don't actually represent new readings!
	//Only save a reading if we have new data (not same as last one)
	//if( g_counts.empty() || (g_counts.back() != sample) )
		g_counts.push_back(sample);

	//reserved (ignore)
	ReadUint16(uart);

	//checksum (ignore)
	ReadUint16(uart);
}

uint16_t ReadUint16(UART& uart)
{
	uint8_t tmp[2];
	uart.Read(tmp, 2);

	uint16_t ret = tmp[0];
	ret = (ret << 8) | tmp[1];

	return ret;
}
