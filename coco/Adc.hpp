#pragma once


namespace coco {

/**
	Simple ADC interface for sampling a value
*/
class Adc {
public:
	virtual ~Adc() {};

	/**
		Sample the value of a channel
		@param channel channel to sample
		@return samled value, typically 0 - 65535 or -32768 - 32767 where actual resolution is platform dependent
	*/
	virtual int get(int channel) = 0;
};

} // namespace coco
