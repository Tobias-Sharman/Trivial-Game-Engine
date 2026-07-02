#ifndef TRIVIAL_LAYERS_H
#define TRIVIAL_LAYERS_H

namespace trivial {

class Layer {
public:
	Layer() = default;

	virtual ~Layer() = default;

	Layer(const Layer&) = delete;
	Layer& operator=(const Layer&) = delete;

	Layer(Layer&&) = delete;
	Layer& operator=(Layer&&) = delete;

	virtual void onStart() {};
	virtual void onUpdate() {};
	virtual void onEnd() {};
};

} // namespace trivial

#endif // TRIVIAL_LAYERS_H
