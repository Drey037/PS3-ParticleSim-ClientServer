import java.awt.*;
import java.util.ArrayList;
import java.util.Random;

public class Particle {
    private final int size = 3;

    private int x, y; // coordinates
    private double preciseX, preciseY; // for lower velocities

    private Color color;

    private double velocity;
    private double theta;

    // Screen size for boundary detection
    private static final int SCREEN_WIDTH = 1280;
    private static final int SCREEN_HEIGHT = 720;


    private static final Random random = new Random(); // for generating random color

    public Particle(int x, int y, double velocity, double theta){
        this.x = x;
        this.y = y;
        this.velocity = velocity;
        this.theta = Math.toRadians(theta);
        this.color = getRandomDarkColor();
        this.preciseX = x;
        this.preciseY = y;
    }

    public void update(double deltaTime) {
        // Convert deltaTime from milliseconds to seconds
        double deltaTimeSeconds = deltaTime / 1000.0;

        // Update position based on velocity and direction, factoring in the elapsed time
        preciseX += (velocity * Math.cos(this.theta)) * deltaTimeSeconds;
        preciseY += (velocity * Math.sin(this.theta)) * deltaTimeSeconds;

        this.x = (int)Math.round(preciseX);
        this.y = (int)Math.round(preciseY);
        // For screen bounds
        if (x <= 0 || x >= SCREEN_WIDTH) {
            theta = Math.PI - theta; // Reflect horizontally
            x = Math.max(0, Math.min(x, SCREEN_WIDTH)); // Keep within bounds
        }
        if (y <= 0 || y >= SCREEN_HEIGHT) {
            theta = -theta; // Reflect vertically
            y = Math.max(0, Math.min(y, SCREEN_HEIGHT)); // Keep within bounds
        }

    }


    public static Color getRandomDarkColor(){
        int r = random.nextInt(128); // Red
        int g = random.nextInt(128); // Green
        int b = random.nextInt(128); // Blue

        return new Color(r, g, b);
    }

    public int getX() {
        return x;
    }

    public int getY() {
        return y;
    }

    public int getSize() {
        return size;
    }

    public Color getColor() {
        return color;
    }

    public void draw (Graphics g){
        g.setColor(color);
        g.fillOval(x, SCREEN_HEIGHT - y, size, size);
    }
}
