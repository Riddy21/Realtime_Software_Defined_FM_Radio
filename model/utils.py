import numpy as np
def compute_lcm(x, y):
    lcm = (x*y)//compute_gcd(x,y)
    return lcm

def compute_gcd(x, y):
    while(y):
        x, y = y, x % y
    return x

def up_sample(samples, sample_rate):
    """Up samples the samples list by the sample rate"""
    new_samples = np.zeros(shape = len(samples)*sample_rate)

    if sample_rate == 1:
        return samples
    else:
        for i in range(len(samples)*sample_rate):
            if (i % sample_rate == 0):
                new_samples[i] = samples[int(i/sample_rate)]
        return np.array(new_samples)
            
