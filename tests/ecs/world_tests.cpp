#include <gtest/gtest.h>

#include <trivial/world/world.h>

namespace {

struct CustomComponent {
	int value = 0;
};

} // namespace

TEST(WorldTests, CreatesAliveEntity) {
	trivial::world::World world;

	const trivial::ecs::Entity kEntity = world.create();

	EXPECT_TRUE(kEntity.valid());
	EXPECT_TRUE(world.alive(kEntity));
	EXPECT_EQ(world.aliveCount(), 1);
	EXPECT_EQ(world.capacity(), 1);
}

TEST(WorldTests, DestroysEntity) {
	trivial::world::World world;

	const trivial::ecs::Entity kEntity = world.create();

	world.destroy(kEntity);

	EXPECT_FALSE(world.alive(kEntity));
	EXPECT_EQ(world.aliveCount(), 0);
	EXPECT_EQ(world.capacity(), 1);
}

TEST(WorldTests, ReusesDestroyedEntitySlotWithNewGeneration) {
	trivial::world::World world;

	const trivial::ecs::Entity kFirst = world.create();

	world.destroy(kFirst);

	const trivial::ecs::Entity kSecond = world.create();

	EXPECT_EQ(kSecond.index(), kFirst.index());
	EXPECT_NE(kSecond.generation(), kFirst.generation());

	EXPECT_FALSE(world.alive(kFirst));
	EXPECT_TRUE(world.alive(kSecond));
	EXPECT_EQ(world.aliveCount(), 1);
}

TEST(WorldTests, StoresStandardComponents) {
	trivial::world::World world;

	const trivial::ecs::Entity kEntity = world.create();

	world.add<trivial::ecs::Position2D>(kEntity, trivial::ecs::Position2D{});
	world.add<trivial::ecs::Velocity2D>(kEntity, trivial::ecs::Velocity2D{});

	EXPECT_TRUE(world.has<trivial::ecs::Position2D>(kEntity));
	EXPECT_TRUE(world.has<trivial::ecs::Velocity2D>(kEntity));
}

TEST(WorldTests, ReadsStandardComponents) {
	constexpr float kX = 2.0f;
	constexpr float kY = 3.0f;

	trivial::world::World world;

	const trivial::ecs::Entity kEntity = world.create();

	trivial::ecs::Position2D position{trivial::math::Vec2f{kX, kY}};

	world.add<trivial::ecs::Position2D>(kEntity, position);

	ASSERT_TRUE(world.has<trivial::ecs::Position2D>(kEntity));

	const trivial::ecs::Position2D& storedPosition = world.get<trivial::ecs::Position2D>(kEntity);

	EXPECT_FLOAT_EQ(storedPosition.value.x, kX);
	EXPECT_FLOAT_EQ(storedPosition.value.y, kY);
}

TEST(WorldTests, StoresCustomComponents) {
	constexpr int kValue = 42;

	trivial::world::World world;

	const trivial::ecs::Entity kEntity = world.create();

	world.add<CustomComponent>(kEntity, CustomComponent{kValue});

	ASSERT_TRUE(world.has<CustomComponent>(kEntity));

	const CustomComponent& component = world.get<CustomComponent>(kEntity);

	EXPECT_EQ(component.value, kValue);
}

TEST(WorldTests, RemovesStandardComponent) {
	trivial::world::World world;

	const trivial::ecs::Entity kEntity = world.create();

	world.add<trivial::ecs::Position2D>(kEntity, trivial::ecs::Position2D{});

	ASSERT_TRUE(world.has<trivial::ecs::Position2D>(kEntity));

	world.remove<trivial::ecs::Position2D>(kEntity);

	EXPECT_FALSE(world.has<trivial::ecs::Position2D>(kEntity));
}

TEST(WorldTests, RemovesCustomComponent) {
	constexpr int kValue = 42;

	trivial::world::World world;

	const trivial::ecs::Entity kEntity = world.create();

	world.add<CustomComponent>(kEntity, CustomComponent{kValue});

	ASSERT_TRUE(world.has<CustomComponent>(kEntity));

	world.remove<CustomComponent>(kEntity);

	EXPECT_FALSE(world.has<CustomComponent>(kEntity));
}

TEST(WorldTests, DestroyRemovesComponentsBeforeReusingEntitySlot) {
	constexpr int kValue = 42;

	trivial::world::World world;

	const trivial::ecs::Entity kFirst = world.create();

	world.add<trivial::ecs::Position2D>(kFirst, trivial::ecs::Position2D{});
	world.add<trivial::ecs::Velocity2D>(kFirst, trivial::ecs::Velocity2D{});
	world.add<CustomComponent>(kFirst, CustomComponent{kValue});

	world.destroy(kFirst);

	const trivial::ecs::Entity kSecond = world.create();

	ASSERT_EQ(kSecond.index(), kFirst.index());
	ASSERT_NE(kSecond.generation(), kFirst.generation());

	EXPECT_FALSE(world.has<trivial::ecs::Position2D>(kSecond));
	EXPECT_FALSE(world.has<trivial::ecs::Velocity2D>(kSecond));
	EXPECT_FALSE(world.has<CustomComponent>(kSecond));
}

TEST(WorldTests, IgnoresComponentsAddedToDestroyedEntity) {
	constexpr int kValue = 42;

	trivial::world::World world;

	const trivial::ecs::Entity kEntity = world.create();

	world.destroy(kEntity);

	world.add<trivial::ecs::Position2D>(kEntity, trivial::ecs::Position2D{});
	world.add<CustomComponent>(kEntity, CustomComponent{kValue});

	EXPECT_FALSE(world.has<trivial::ecs::Position2D>(kEntity));
	EXPECT_FALSE(world.has<CustomComponent>(kEntity));
}
