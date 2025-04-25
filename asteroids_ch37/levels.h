#pragma once
struct level {
	int nums[4]; // how many of each size of asteroid
	int aliens; // how many aliens
	float factor; // from 1.0 to 1.5 - multiply astereoid speed by this
};

struct level levels[50];