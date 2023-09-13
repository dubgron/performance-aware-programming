#include <stdio.h>
#include <stdlib.h>

typedef double              f64;
typedef unsigned long long  u64;

static f64 Square(f64 A)
{
    f64 Result = (A*A);
    return Result;
}

static f64 RadiansFromDegrees(f64 Degrees)
{
    f64 Result = 0.01745329251994329577 * Degrees;
    return Result;
}

// NOTE(casey): EarthRadius is generally expected to be 6372.8
static f64 ReferenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1, f64 EarthRadius)
{
    /* NOTE(casey): This is not meant to be a "good" way to calculate the Haversine distance.
       Instead, it attempts to follow, as closely as possible, the formula used in the real-world
       question on which these homework exercises are loosely based.
    */
    
    f64 lat1 = Y0;
    f64 lat2 = Y1;
    f64 lon1 = X0;
    f64 lon2 = X1;
    
    f64 dLat = RadiansFromDegrees(lat2 - lat1);
    f64 dLon = RadiansFromDegrees(lon2 - lon1);
    lat1 = RadiansFromDegrees(lat1);
    lat2 = RadiansFromDegrees(lat2);
    
    f64 a = Square(sin(dLat/2.0)) + cos(lat1)*cos(lat2)*Square(sin(dLon/2));
    f64 c = 2.0*asin(sqrt(a));
    
    f64 Result = EarthRadius * c;
    
    return Result;
}

void random_point(f64* x, f64* y)
{
    // [-180, 180]
    *x = (f64)((rand()*rand()) % 3600000) / 10000.0 - 180.0;

    // [-90, 90]
    *y = (f64)((rand()*rand()) % 1800000) / 10000.0 - 90.0;
}

int main(int argv, char **argc)
{
    if (argv < 2)
    {
	printf("usage: haversine_json_generator number_of_pairs_to_generate [random_seed]\n");
	return 1;
    }
    u64 samples = atoi(argc[1]);

    u64 seed = time(0);
    if (argv > 2)
    {
	seed = strtod(argc[2], NULL);
    }
    srand(seed);
    
    printf("Generating %llu random pairs of coordinate points with seed %llu!\n",
	   samples, seed);
    
    FILE *out_file = fopen("out/haversine.json", "w");

    const char header[] = "{ \"points\": [\n";
    fwrite(header, sizeof(header) - 1, 1, out_file);
    
    for (u64 idx = 0; idx < samples; ++idx)
    {
	f64 x0, y0, x1, y1;
	random_point(&x0, &y0);
	random_point(&x1, &y1);

	fprintf(out_file, "{ \"x0\": %f, \"y0\": %f, \"x1\": %f, \"y1\": %f }%s\n",
		x0, y0, x1, y1, idx != samples - 1 ? "," : "");
    }

    const char footer[] = "] }\n";
    fwrite(footer, sizeof(footer) - 1, 1, out_file);
    
    return 0;
}
