import javax.imageio.ImageIO;
import javax.swing.*;
import javax.swing.border.LineBorder;
import java.awt.*;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;

public class ParticlePanel extends JPanel {
    private long lastUpdateTime;
    private int frames;
    private JLabel fpsLabel;
    private ArrayList<ParticleBatch> particleBatchList;

    private Image texture_left, texture_right;

    // CANVAS SETTINGS
    private final int ROW = 19;
    private final int COL = 33;
    private final int WIDTH = 1280;
    private final int HEIGHT = 720;
    private final int CHAR_MAP_WIDTH = 39;
    private final int CHAR_MAP_HEIGHT = 37;

    private final int ZOOMX = WIDTH / COL;
    private final int ZOOMY = HEIGHT / ROW;

    private Ghost character;

    private ArrayList<Ghost> Buddies;

    public ParticlePanel(ArrayList<ParticleBatch> particleBatchList, Ghost character, ArrayList<Ghost> Buddies) {
        this.particleBatchList = particleBatchList;
        setPreferredSize(new Dimension(1280, 720));
        setBackground(Color.BLACK);
        setLayout(null); // Use null layout to manually position components

        // FPS Label
        fpsLabel = new JLabel("FPS: 0");
        fpsLabel.setForeground(new Color(173, 200, 220));
        fpsLabel.setHorizontalAlignment(SwingConstants.RIGHT);
        add(fpsLabel);

        this.character = character;
        this.Buddies = Buddies;

    }
    private int translateX(int cameraX) {
        int MapX = -1;
        // Too far left
        if (cameraX <= 0) {
            MapX = -(cameraX * ZOOMX); // Ensure MapX is not less than 0
        }
        // Too far right
        else if (cameraX + COL > WIDTH) {
            MapX = -((cameraX - WIDTH + COL) * ZOOMX);
        }
        else if (cameraX > 0 && cameraX <= WIDTH - COL){
            MapX = 0;
        }
        return MapX;
    }

    private int translateY(int cameraY) {
        int MapY = -1;

        if (cameraY <= 0) {
            MapY = -(cameraY * ZOOMY); // Ensure MapX is not less than 0
        }
        if (cameraY + ROW >= HEIGHT) {
            MapY = -((cameraY - HEIGHT + ROW) * ZOOMY);
        }
        else if (cameraY > 0 && cameraY <= HEIGHT - ROW){
            MapY = 0;
        }
        return MapY;
    }


    @Override
    protected void paintComponent(Graphics g) {
        super.paintComponent(g);
        // Create a Graphics2D object from the Graphics object
        Graphics2D g2d = (Graphics2D) g.create();
        // Set the background color to white

        int cameraX = character.getX() - COL / 2;
        int cameraY = HEIGHT - character.getY() - ROW / 2;

        //translateMap(g2d);
        int MapX = translateX(cameraX);
        int MapY = translateY(cameraY);
        g2d.translate(MapX, MapY);
        g2d.setColor(Color.WHITE);
        g2d.fillRect(0, 0, getWidth(), getHeight());

        // Draw particles
        for (ParticleBatch batch : particleBatchList) {
            ArrayList<Particle> particleList = batch.getParticles();
            for (Particle particle : particleList) {
                // Calculate the particle's position relative to the camera's position
                int pX = particle.getX() - cameraX;
                int pY = HEIGHT - particle.getY() - cameraY;

                // Check if the particle is within the viewport
                if (pX >= 0 && pX < COL && pY >= 0 && pY < ROW) {
                    pX = pX * ZOOMX;
                    pY = pY * ZOOMY;
                    g.setColor(particle.getColor());
                    g.fillRect(pX, pY, particle.getSize() * 4, particle.getSize() * 4);
                }
            }
        }

        // Draw the character
        character.drawMap(g); // Use the transformed Graphics2D object

        for (Ghost buddy: Buddies) {
            int ghostX = buddy.getX() - cameraX;
            int ghostY = HEIGHT - buddy.getY() - cameraY;
            Boolean buddyIsLeft = buddy.getIsLeft();

            if (ghostX >= 0 && ghostX < COL && ghostY >= 0 && ghostY < ROW) {
                int ghostXZoom = ghostX * ZOOMX;
                int ghostYZoom = ghostY * ZOOMY;
                if (buddyIsLeft) {
                    g.drawImage(texture_left, ghostXZoom, ghostYZoom, CHAR_MAP_WIDTH, CHAR_MAP_HEIGHT, null);
                }
                else {
                    g.drawImage(texture_right, ghostXZoom, ghostYZoom, CHAR_MAP_WIDTH, CHAR_MAP_HEIGHT, null);
                }
            }
        }

        // Dispose of the Graphics2D object to free up resources
        g2d.dispose();
        // Update FPS label position dynamically
        fpsLabel.setBounds(getWidth() - 110, getHeight() - 30, 100, 20);

        // Run a function after paintComponent is done
        SwingUtilities.invokeLater(this::runFPSCounter);

    }

    private void runFPSCounter() {
        long now = System.currentTimeMillis();
        long elapsed = now - lastUpdateTime;
        frames++;

        if (elapsed >= 500) { // 0.5 seconds
            final int fps = (int) ((frames * 1000) / elapsed);
            SwingUtilities.invokeLater(() -> fpsLabel.setText("EM FPS: " + fps));
            frames = 0;
            lastUpdateTime = now;
        }
        try {
            Thread.sleep(Math.max(1, (1000 / 60) - (System.currentTimeMillis() - now)));
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }
}