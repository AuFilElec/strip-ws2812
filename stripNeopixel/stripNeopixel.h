// Color scheme definitions.
struct sColor {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  
  sColor(uint8_t red, uint8_t green, uint8_t blue): red(red), green(green), blue(blue) {}
  sColor(): red(0), green(0), blue(0) {}
};
typedef sColor Color;

struct sColorScheme {
  Color* colors;
  uint8_t count;
 
  sColorScheme(Color* colors, uint8_t count): colors(colors), count(count) {} 
};
typedef sColorScheme ColorScheme;
