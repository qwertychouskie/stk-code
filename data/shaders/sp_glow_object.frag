flat in vec4 color_data;

out vec4 FragColor;

void main()
{
    FragColor = vec4(color_data.bgr, 1.0);
}
