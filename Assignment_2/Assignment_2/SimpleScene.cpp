#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <cmath>
#include <GL/glut.h>
////////////////////////////////////////modified//////////////////////////////////////////
#include <random>
#include <cmath>
////////////////////////////////////////modified//////////////////////////////////////////
#include "FrameXform.h"
#include "wavefront_obj.h"


using double2 = std::array<double, 2>;
using double3 = std::array<double, 3>;

// type-invariant PI 
template<typename T>
T const PI = std::acos( -T( 1 ) );

// five camera info
std::vector<std::array<double, 9>> cameras
{
    {28, 18, 28, 0, 2, 0, 0, 1, 0},
    {28, 18, -28, 0, 2, 0, 0, 1, 0},
    {-28, 18, 28, 0, 2, 0, 0, 1, 0},
    {-12, 12, 0, 0, 2, 0, 0, 1, 0},
    {0, 100, 0,  0, 0, 0, 1, 0, 0}
};

int cameraIndex, camID;
std::vector<FrameXform> wld2cam, cam2wld;
wavefront_obj_t *cam;

// Variables for 'cow' object.
FrameXform cow2wld;
wavefront_obj_t *cow;
int cowID;

// Variables for 'beethovan' object.
FrameXform bet2wld;
wavefront_obj_t *bet;
int betID;

unsigned floorTexID;
int frame = 0;
int width, height;
int selectMode, oldX, oldY;

void drawFrame( float len );

////////////////////////////////////////modified//////////////////////////////////////////

enum ObjectMode {FIRST_MODE, SECOND_MODE};
enum InputKey {key_x, key_y, key_z, key_r, key_p, key_d, key_o, key_t};
enum SelectMode {SELECT_ON, SELECT_OFF};
enum BufferMode {FRONT_BUFFER, BACK_BUFFER};
enum SelectedObject {NONE_SELECTED, COW_SELECTED, BEETHOVAN_SELECTED};

bool isAutoRotating = false;
SelectMode nowSelectMode = SELECT_OFF;
BufferMode nowBufferMode = FRONT_BUFFER;
InputKey nowInputKey;
ObjectMode nowObjectMode = FIRST_MODE;
SelectedObject selectedObject = COW_SELECTED;
int amountOfChange = 0;

double amountOfRotateX;
double amountOfRotateY;
double amountOfRotateZ;

std::random_device rn;
std::mt19937 gen(rn());
std::uniform_real_distribution<> range(0, 10);
////////////////////////////////////////modified//////////////////////////////////////////

double3 munge( int x )
{
	double r, g, b;
    r = ( x & 255 ) / double( 255 );
    g = ( ( x >> 8 ) & 255 ) / double( 255 );
    b = ( ( x >> 16 ) & 255 ) / double( 255 );
	return double3{ r, g, b };
}

int unmunge( double3 color )
{
	double r = color[0], g = color[1], b = color[2];
    return ( int( r ) + ( int( g ) << 8 ) + ( int( b ) << 16 ) );
}

void setCamera()
{
    int i;
    if ( frame == 0 )
	{
        // intialize camera model.
        cam = new wavefront_obj_t( "camera.obj" );	// Read from camera.obj.
        camID = glGenLists( 1 );					// Create display list for the camera.
        glNewList( camID, GL_COMPILE );				// Begin compiling the display list using camID.
		cam->draw();								// Draw the camera. you can do this job again through camID..
        glEndList();								// Terminate compiling the display list.

        // initialize camera frame transforms.
        for ( i = 0; i < cameras.size(); i++ )
		{
            auto &camera = cameras[i];										// pointer to information of i-th camera.
            wld2cam.push_back( FrameXform() );								// Insert {0} matrix to wld2cam
            glPushMatrix();													// Push the current matrix of GL into stack.
            glLoadIdentity();												// Set the GL matrix Identity matrix.
            gluLookAt( camera[0], camera[1], camera[2], camera[3], camera[4], camera[5], camera[6], camera[7], camera[8] );		// Multiply a LookAt matrix computed by gluLookAt function to the current matrix.
            glGetDoublev( GL_MODELVIEW_MATRIX, wld2cam[i].matrix() );		// Read the world-to-camera matrix computed by gluLookAt.
            glPopMatrix();													// Pop the matrix that was pushed the stack to GL.
            cam2wld.push_back( wld2cam[i].inverse() );						// Get the camera-to-world matrix.
        }
        cameraIndex = 0;
    }

    // set viewing transformation.
    glLoadMatrixd( wld2cam[cameraIndex].matrix() );

    // draw other cameras.
    for ( i = 0; i < ( int )wld2cam.size(); i++ )
	{
        if ( i != cameraIndex )
		{
            glPushMatrix();												// Push the current matrix of GL to stack. The current matrix is wld2cam[cameraIndex].matrix().
            glMultMatrixd( cam2wld[i].matrix() );						// Multiply the matrix to draw i-th camera.
            if ( selectMode == 0 )										// selectMode == 1 indicates backbuffer mode.
			{									
                drawFrame( 5 );											// Draw x, y, and z axis.
                float frontColor[] = {0.2f, 0.2f, 0.2f, 1.0f};
                glEnable( GL_LIGHTING );
                glMaterialfv( GL_FRONT, GL_AMBIENT, frontColor );			// Set ambient property frontColor.
                glMaterialfv( GL_FRONT, GL_DIFFUSE, frontColor );			// Set diffuse property frontColor.
            } 
			else
			{
                double3 color;
                glDisable( GL_LIGHTING );									// Disable lighting in backbuffer mode.
                color = munge( i + 1 );										// Match the corresponding (i+1)th color to r, g, b. You can change the color of camera on backbuffer.
                glColor3dv( color.data() );									// Set r, g, b the color of camera.
            }
            glScaled( 0.5, 0.5, 0.5 );										// Scale camera size by 1/2.
            glTranslated( 1.1, 1.1, 0.0 );									// Translate it by (1.1, 1.1, 0.0).
            glCallList( camID );											// Re-draw using display list from camID.
            glPopMatrix();													// Call the matrix on stack. wld2cam[cameraIndex].matrix() in here.
        }
    }
}


/*********************************************************************************
* Draw x, y, z axis of current frame on screen.
* x, y, and z are corresponded Red, Green, and Blue, resp.
**********************************************************************************/
void drawFrame( float len )
{
    glDisable( GL_LIGHTING );		// Lighting is not needed for drawing axis.
    glBegin( GL_LINES );			// Start drawing lines.
    glColor3d( 1, 0, 0 );			// color of x-axis is red.
    glVertex3d( 0, 0, 0 );
    glVertex3d( len, 0, 0 );		// Draw line(x-axis) from (0,0,0) to (len, 0, 0).
    glColor3d( 0, 1, 0 );			// color of y-axis is green.
    glVertex3d( 0, 0, 0 );
    glVertex3d( 0, len, 0 );		// Draw line(y-axis) from (0,0,0) to (0, len, 0).
    glColor3d( 0, 0, 1 );			// color of z-axis is  blue.
    glVertex3d( 0, 0, 0 );
    glVertex3d( 0, 0, len );		// Draw line(z-axis) from (0,0,0) - (0, 0, len).
    glEnd();						// End drawing lines.
}

/*********************************************************************************
* Draw 'cow' object.
**********************************************************************************/
void drawCow() {
    if ( frame == 0 )
	{
        // Initialization part.

        // Read information from cow.obj.
        cow = new wavefront_obj_t( "cow.obj" );

        // Make display list. After this, you can draw cow using 'cowID'.
        cowID = glGenLists( 1 );			// Create display lists
        glNewList( cowID, GL_COMPILE );		// Begin compiling the display list using cowID
        cow->draw();						// Draw the cow on display list.
        glEndList();						// Terminate compiling the display list. Now, you can draw cow using 'cowID'.
        glPushMatrix();						// Push the current matrix of GL into stack.
        glLoadIdentity();					// Set the GL matrix Identity matrix.
        glTranslated( 0, -cow->aabb.first[1], -8 );	// Set the location of cow.
        glRotated( -90, 0, 1, 0 );			// Set the direction of cow. These information are stored in the matrix of GL.
        glGetDoublev( GL_MODELVIEW_MATRIX, cow2wld.matrix() );	// Read the modelview matrix about location and direction set above, and store it in cow2wld matrix.
        glPopMatrix();						// Pop the matrix from stack of GL.
    }

	////////////////////////////////////////modified//////////////////////////////////////////
	if (nowObjectMode == FIRST_MODE && nowInputKey == key_x && selectedObject == COW_SELECTED) {
		if (amountOfChange != 0) {
			glPushMatrix();
			glLoadMatrixd(cow2wld.matrix());
			glTranslated(amountOfChange, 0, 0);
			glGetDoublev(GL_MODELVIEW_MATRIX, cow2wld.matrix());
			glPopMatrix();
		}
	} else if (nowObjectMode == FIRST_MODE && nowInputKey == key_y && selectedObject == COW_SELECTED) {
		if (amountOfChange != 0) {
			glPushMatrix();
			glLoadMatrixd(cow2wld.matrix());
			glTranslated(0, amountOfChange, 0);
			glGetDoublev(GL_MODELVIEW_MATRIX, cow2wld.matrix());
			glPopMatrix();
		}
	}
	else if (nowObjectMode == FIRST_MODE && nowInputKey == key_z && selectedObject == COW_SELECTED) {
		if (amountOfChange != 0) {
			glPushMatrix();
			glLoadMatrixd(cow2wld.matrix());
			glTranslated(0, 0, amountOfChange);
			glGetDoublev(GL_MODELVIEW_MATRIX, cow2wld.matrix());
			glPopMatrix();
		}
	}
	if (nowObjectMode == FIRST_MODE && isAutoRotating && selectedObject == COW_SELECTED) {
		glPushMatrix();
		glLoadMatrixd(cow2wld.matrix());
		glRotated(1, amountOfRotateX, amountOfRotateY, amountOfRotateZ);
		glGetDoublev(GL_MODELVIEW_MATRIX, cow2wld.matrix());

		glPopMatrix();

	}
	////////////////////////////////////////modified//////////////////////////////////////////


	glPushMatrix();		// Push the current matrix of GL into stack. This is because the matrix of GL will be change while drawing cow.

	glMultMatrixd(cow2wld.matrix());
	// The information about location of cow to be drawn is stored in cow2wld matrix.
    // (Project #1 hint) If you change the value of the cow2wld matrix or the current matrix, cow would rotate or move.
	
    if ( selectMode == 0 )			// selectMode == 1 indicates backbuffer mode.
	{
		////////////////////////////////////////modified//////////////////////////////////////////			
		if (nowObjectMode == FIRST_MODE && isAutoRotating && selectedObject == COW_SELECTED) {
			double sqrtValue = std::sqrt(amountOfRotateX * amountOfRotateX + amountOfRotateY * amountOfRotateY + amountOfRotateZ * amountOfRotateZ) / 5;

			glBegin(GL_LINES);			// Start drawing lines.
			glColor3d(1, 1, 1);

			glVertex3d(amountOfRotateX / sqrtValue, amountOfRotateY / sqrtValue, amountOfRotateZ / sqrtValue);
			glVertex3d(-amountOfRotateX / sqrtValue, -amountOfRotateY / sqrtValue, -amountOfRotateZ / sqrtValue);
			glEnd();
		}
		////////////////////////////////////////modified//////////////////////////////////////////
        drawFrame( 5 );										// Draw x, y, and z axis.
        float frontColor[] = {0.8f, 0.2f, 0.9f, 1.0f};
        glEnable( GL_LIGHTING );
        glMaterialfv( GL_FRONT, GL_AMBIENT, frontColor );		// Set ambient property frontColor.
        glMaterialfv( GL_FRONT, GL_DIFFUSE, frontColor );		// Set diffuse property frontColor.
    }
	else
	{
		////////////////////////////////////////modified//////////////////////////////////////////			
		if (nowObjectMode == FIRST_MODE && isAutoRotating && selectedObject == COW_SELECTED) {
			double sqrtValue = std::sqrt(amountOfRotateX * amountOfRotateX + amountOfRotateY * amountOfRotateY + amountOfRotateZ * amountOfRotateZ) / 5;

			glBegin(GL_LINES);			// Start drawing lines.
			glColor3d(1.0, 1.0, 1.0);

			glVertex3d(amountOfRotateX / sqrtValue, amountOfRotateY / sqrtValue, amountOfRotateZ / sqrtValue);
			glVertex3d(-amountOfRotateX / sqrtValue, -amountOfRotateY / sqrtValue, -amountOfRotateZ / sqrtValue);
			glEnd();
		}
		////////////////////////////////////////modified//////////////////////////////////////////
        glDisable( GL_LIGHTING );								// Disable lighting in backbuffer mode.
		double3 color = munge( 255 );							// Match the corresponding constant color to r, g, b. You can change the color of camera on backbuffer
        glColor3dv( color.data() );
    }
    glCallList( cowID );		// Draw cow.
    glPopMatrix();				// Pop the matrix in stack to GL. Change it the matrix before drawing cow.

	////////////////////////////////////////modified//////////////////////////////////////////
	if (nowObjectMode == FIRST_MODE && isAutoRotating && selectedObject == COW_SELECTED) {
		glutPostRedisplay();
	}
	////////////////////////////////////////modified//////////////////////////////////////////
}

/*********************************************************************************
* Draw 'beethovan' object.
**********************************************************************************/
void drawBet()
{
    if ( frame == 0 )
	{
        // Initialization part.

        // Read information from beethovan.obj.
        bet = new wavefront_obj_t( "beethovan.obj" );

        // Make display list. After this, you can draw beethovan using 'betID'.
        betID = glGenLists( 1 );
        glNewList( betID, GL_COMPILE );
        bet->draw();
        glEndList();
        glPushMatrix();
        glLoadIdentity();
        glTranslated( 0, -bet->aabb.first[1], 8 );
        glRotated( 180, 0, 1, 0 );
        glGetDoublev( GL_MODELVIEW_MATRIX, bet2wld.matrix() );
        glPopMatrix();
    }
	////////////////////////////////////////modified//////////////////////////////////////////
	if (nowObjectMode == FIRST_MODE && nowInputKey == key_x && selectedObject == BEETHOVAN_SELECTED) {
		if (amountOfChange != 0) {
			glPushMatrix();
			glLoadMatrixd(bet2wld.matrix());
			glTranslated(amountOfChange, 0, 0);
			glGetDoublev(GL_MODELVIEW_MATRIX, bet2wld.matrix());
			glPopMatrix();
		}
	}
	else if (nowObjectMode == FIRST_MODE && nowInputKey == key_y && selectedObject == BEETHOVAN_SELECTED) {
		if (amountOfChange != 0) {
			glPushMatrix();
			glLoadMatrixd(bet2wld.matrix());
			glTranslated(0, amountOfChange, 0);
			glGetDoublev(GL_MODELVIEW_MATRIX, bet2wld.matrix());
			glPopMatrix();
		}
	}
	else if (nowObjectMode == FIRST_MODE && nowInputKey == key_z && selectedObject == BEETHOVAN_SELECTED) {
		if (amountOfChange != 0) {
			glPushMatrix();
			glLoadMatrixd(bet2wld.matrix());
			glTranslated(0, 0, amountOfChange);
			glGetDoublev(GL_MODELVIEW_MATRIX, bet2wld.matrix());
			glPopMatrix();
		}
	}
	if (nowObjectMode == FIRST_MODE && isAutoRotating && selectedObject == BEETHOVAN_SELECTED) {
		glPushMatrix();
		glLoadMatrixd(bet2wld.matrix());
		glRotated(1, amountOfRotateX, amountOfRotateY, amountOfRotateZ);
		glGetDoublev(GL_MODELVIEW_MATRIX, bet2wld.matrix());
		glPopMatrix();
	}
	////////////////////////////////////////modified//////////////////////////////////////////

    glPushMatrix();
    glMultMatrixd( bet2wld.matrix() );
    if ( selectMode == 0 )
	{
		////////////////////////////////////////modified//////////////////////////////////////////			
		if (nowObjectMode == FIRST_MODE && isAutoRotating && selectedObject == BEETHOVAN_SELECTED) {
			double sqrtValue = std::sqrt(amountOfRotateX * amountOfRotateX + amountOfRotateY * amountOfRotateY + amountOfRotateZ * amountOfRotateZ) / 8;

			glDisable(GL_LIGHTING);
			glColor3d(1.0f, 1.0f, 1.0f);

			glBegin(GL_LINES);			// Start drawing lines.

			glVertex3d(amountOfRotateX / sqrtValue, amountOfRotateY / sqrtValue, amountOfRotateZ / sqrtValue);
			glVertex3d(-amountOfRotateX / sqrtValue, -amountOfRotateY / sqrtValue, -amountOfRotateZ / sqrtValue);
			
			glEnd();
		}
		////////////////////////////////////////modified//////////////////////////////////////////
        drawFrame( 8 );
        float frontColor[] = {0.8, 0.3, 0.1, 1.0};
        glEnable( GL_LIGHTING );
        glMaterialfv( GL_FRONT, GL_AMBIENT, frontColor );
        glMaterialfv( GL_FRONT, GL_DIFFUSE, frontColor );
    }
	else
	{
		////////////////////////////////////////modified//////////////////////////////////////////			
		if (nowObjectMode == FIRST_MODE && isAutoRotating && selectedObject == BEETHOVAN_SELECTED) {
			double sqrtValue = std::sqrt(amountOfRotateX * amountOfRotateX + amountOfRotateY * amountOfRotateY + amountOfRotateZ * amountOfRotateZ) / 8;

			glColor3d(1.0f, 1.0f, 1.0f);
			glBegin(GL_LINES);			// Start drawing lines.

			glVertex3d(amountOfRotateX / sqrtValue, amountOfRotateY / sqrtValue, amountOfRotateZ / sqrtValue);
			glVertex3d(-amountOfRotateX / sqrtValue, -amountOfRotateY / sqrtValue, -amountOfRotateZ / sqrtValue);
			glEnd();
		}
		////////////////////////////////////////modified//////////////////////////////////////////
        glDisable( GL_LIGHTING );
        double3 color = munge( 65025 );
        glColor3dv(color.data() );
    }
    glCallList( betID );
    glPopMatrix();
	////////////////////////////////////////modified//////////////////////////////////////////
	if (nowObjectMode == FIRST_MODE && isAutoRotating && selectedObject == BEETHOVAN_SELECTED) {
		glutPostRedisplay();
	}
	////////////////////////////////////////modified//////////////////////////////////////////
}


/*********************************************************************************
* Draw floor on 3D plane.
**********************************************************************************/
void drawFloor()
{
    if ( frame == 0 )
	{
        // Initialization part.
        // After making checker-patterned texture, use this repetitively.

        // Insert color into checker[] according to checker pattern.
        const int size = 8;
        unsigned char checker[size * size * 3];
        for( int i = 0; i < size * size; i++ )
		{
            if ( ( ( i / size ) ^ i ) & 1 )
			{
                checker[3 * i + 0] = 200;
                checker[3 * i + 1] = 32;
                checker[3 * i + 2] = 32;
            }
			else
			{
                checker[3 * i + 0] = 200;
                checker[3 * i + 1] = 200;
                checker[3 * i + 2] = 32;
            }
        }

        // Make texture which is accessible through floorTexID.
        glGenTextures( 1, &floorTexID );
        glBindTexture( GL_TEXTURE_2D, floorTexID );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
        glTexImage2D( GL_TEXTURE_2D, 0, 3, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, checker );
    }

    glDisable( GL_LIGHTING );

    // Set background color.
    if ( selectMode == 0 )
	{
        glColor3d( 0.35, .2, 0.1 );
	}
    else
	{
        // In backbuffer mode.
		double3 color = munge(34);
		glColor3dv(color.data());
    }

    // Draw background rectangle.
    glBegin( GL_POLYGON );
    glVertex3f( 2000, -0.2, 2000 );
    glVertex3f( 2000, -0.2, -2000 );
    glVertex3f( -2000, -0.2, -2000 );
    glVertex3f( -2000, -0.2, 2000 );
    glEnd();


    // Set color of the floor.
    if ( selectMode == 0 )
	{
        // Assign checker-patterned texture.
        glEnable( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, floorTexID );
    }
	else
	{
        // Assign color on backbuffer mode.
		double3 color = munge(2105376);
		glColor3dv(color.data());
    }

    // Draw the floor. Match the texture's coordinates and the floor's coordinates resp.
    glBegin( GL_POLYGON );
    glTexCoord2d( 0, 0 );
    glVertex3d( -12, -0.1, -12 );		// Texture's (0,0) is bound to (-12,-0.1,-12).
    glTexCoord2d( 1, 0 );
    glVertex3d( 12, -0.1, -12 );		// Texture's (1,0) is bound to (12,-0.1,-12).
    glTexCoord2d( 1, 1 );
    glVertex3d( 12, -0.1, 12 );		// Texture's (1,1) is bound to (12,-0.1,12).
    glTexCoord2d( 0, 1 );
    glVertex3d( -12, -0.1, 12 );		// Texture's (0,1) is bound to (-12,-0.1,12).
    glEnd();

    if ( selectMode == 0 )
	{
        glDisable( GL_TEXTURE_2D );
        drawFrame( 5 );				// Draw x, y, and z axis.
    }
}


/*********************************************************************************
* Call this part whenever display events are needed.
* Display events are called in case of re-rendering by OS. ex) screen movement, screen maximization, etc.
* Or, user can occur the events by using glutPostRedisplay() function directly.
* this part is called in main() function by registering on glutDisplayFunc(display).
**********************************************************************************/
void display()
{
    // selectMode == 1 means backbuffer mode.
    if ( selectMode == 0 )
	{
        glClearColor( 0, 0.6, 0.8, 1 );								// Clear color setting
	}
    else
	{
        glClearColor( 0, 0, 0, 1 );									// When the backbuffer mode, clear color is set to black
	}

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );			// Clear the screen
    setCamera();													// Locate the camera's position, and draw.

    drawFloor();													// Draw floor.
    drawCow();														// Draw cow.
    drawBet();

    glFlush();

    // Not in the backbuffer mode, swap the screen. In backbuffer mode, this is not necessary because it is not presented on screen.
    if ( selectMode == 0 || nowBufferMode == BACK_BUFFER )
        glutSwapBuffers();
    frame += 1;
}


/*********************************************************************************
* Call this part whenever size of the window is changed.
* This part is called in main() function by registering on glutReshapeFunc(reshape).
**********************************************************************************/
void reshape( int w, int h )
{
    width = w;
    height = h;
    glViewport( 0, 0, width, height );
    glMatrixMode( GL_PROJECTION );          // Enable The Projection Matrix
    glLoadIdentity();                       // Reset The Projection Matrix
    // Define perspective projection frustum
    double aspect = width / double( height );
    gluPerspective( 45, aspect, 1, 1024 );
    glMatrixMode( GL_MODELVIEW );           // Enable The Modelview Matrix
    glLoadIdentity();                       // Reset The Projection Matrix
}

void initialize()
{
    // Set up OpenGL state
    glShadeModel( GL_SMOOTH );       // Set Smooth Shading
    glEnable( GL_DEPTH_TEST );       // Enables Depth Testing
    glDepthFunc( GL_LEQUAL );        // The Type Of Depth Test To Do
    // Use perspective correct interpolation if available
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
    // Initialize the matrix stacks
    reshape( width, height );
    // Define lighting for the scene
    float lightDirection[]   = {1.0, 1.0, 1.0, 0};
    float ambientIntensity[] = {0.1, 0.1, 0.1, 1.0};
    float lightIntensity[]   = {0.9, 0.9, 0.9, 1.0};
    glLightfv( GL_LIGHT0, GL_AMBIENT, ambientIntensity );
    glLightfv( GL_LIGHT0, GL_DIFFUSE, lightIntensity );
    glLightfv( GL_LIGHT0, GL_POSITION, lightDirection );
    glEnable( GL_LIGHT0 );
}

/*********************************************************************************
* Call this part whenever mouse button is clicked.
* This part is called in main() function by registering on glutMouseFunc(onMouseButton).
**********************************************************************************/
void onMouseButton( int button, int state, int x, int y )
{
    y = height - y - 1;
    if ( button == GLUT_LEFT_BUTTON )
	{
        if ( state == GLUT_DOWN )
		{
            printf( "Left mouse click at (%d, %d)\n", x, y );

            // (Project #2) After drawing object on backbuffer, you can distinguish which object is selected by reading pixel of (x, y).
            // If you change the value of selectMode to 1, then draw the object on backbuffer when display() function is called.
			
            selectMode = 1;
            display();
            glReadBuffer( GL_BACK );
            unsigned char pixel[3];
            glReadPixels( x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel );
			double3 pixel_d3 = { double(pixel[0]), double(pixel[1]), double(pixel[2]) };
			printf("pixel = %d\n", unmunge(pixel_d3));
			if (nowBufferMode != BACK_BUFFER)
				selectMode = 0;

            // (Project #2) TODO : Perform the proper task about selected object.
			////////////////////////////////////////modified//////////////////////////////////////////
			if (nowSelectMode == SELECT_ON) {
				switch (unmunge(pixel_d3)) {
				case 255:
					selectedObject = COW_SELECTED;
					printf("Cow_Selected\n");
					break;
				case 65025:
					selectedObject = BEETHOVAN_SELECTED;
					printf("Beethovan_Selected\n");
					break;
				default:
					selectedObject = NONE_SELECTED;
					printf("Nothing_Selected\n");
				}
			}
			////////////////////////////////////////modified//////////////////////////////////////////
            // Save recently clicked location.
            oldX = x;
            oldY = y;
        }
    }
	else if ( button == GLUT_RIGHT_BUTTON )
	{
        printf( "Right mouse click at (%d, %d)\n", x, y );
    }
	else{}

    glutPostRedisplay();
}



/*********************************************************************************
* Call this part whenever user drags mouse.
* Input parameters x, y are coordinate of mouse on dragging.
* Value of global variables oldX, oldY is stored on onMouseButton,
* Then, those are used to verify value of x - oldX,  y - oldY to know its movement.
**********************************************************************************/
void onMouseDrag( int x, int y )
{
    y = height - y - 1;
    printf( "in drag (%d, %d)\n", x - oldX,  y - oldY );

    // (Project #1,#2) TODO : Implemention for mouse drag events.
	////////////////////////////////////////modified//////////////////////////////////////////
	amountOfChange = (y - oldY + x - oldX) / 8 ;

	oldX = x;
	oldY = y;
	////////////////////////////////////////modified//////////////////////////////////////////
    glutPostRedisplay();
}

/*********************************************************************************
* Call this part whenever user types keyboard.
* This part is called in main() function by registering on glutKeyboardFunc(onKeyPress).
**********************************************************************************/
void onKeyPress( unsigned char key, int x, int y )
{
    // If 'c' or space bar are pressed, alter the camera.
    // If a number is pressed, alter the camera corresponding the number.

    if ( ( key == ' ' ) || ( key == 'c' ) )
	{
        printf( "Toggle camera %d\n", cameraIndex );
        cameraIndex += 1;
    }
	else if ( ( key >= '0' ) && ( key <= '9' ) )
	{
        cameraIndex = key - '0';
	}
	else{}

    if ( cameraIndex >= ( int )wld2cam.size() )
	{
        cameraIndex = 0;
	}

    // (Project #1, #2) TODO : Implementation for keyboard input.
	////////////////////////////////////////modified//////////////////////////////////////////

	if (key == 'r') {
		nowInputKey = key_r;
		if (nowObjectMode == FIRST_MODE) {
			if (isAutoRotating)\
				isAutoRotating = false;
			else {
				isAutoRotating = true;

				while (true) {
					amountOfRotateX = range(gen);
					amountOfRotateY = range(gen);
					amountOfRotateZ = range(gen);
					if (amountOfRotateX != 0 || amountOfRotateY != 0 || amountOfRotateZ != 0) break;
				}
			}
		}
	}
	else if (key == 'x') {
		nowInputKey = key_x;
	}
	else if (key == 'y') {
		nowInputKey = key_y;
	}
	else if (key == 'z') {
		nowInputKey = key_z;
	}
	else if (key == 'v') {
		if(nowObjectMode == FIRST_MODE) nowObjectMode = SECOND_MODE;
		else nowObjectMode = FIRST_MODE;
	}
	else if (key == 'p') {
		nowInputKey = key_p;
	}
	else if (key == 'd') {
		nowInputKey = key_d;
	}
	else if (key == 'o') {
		nowInputKey = key_o;
	}
	else if (key == 't') {
		nowInputKey = key_t;
	}
	else {
		if (key == 's') {
			if (nowSelectMode == SELECT_OFF) {
				nowSelectMode = SELECT_ON;
				printf("SelectMode On\n");
			}
			else {
				nowSelectMode = SELECT_OFF;
				printf("SelectMode Off\n");
			}
		}
		else if (key == 'b') {
			if (nowBufferMode == FRONT_BUFFER) {
				nowBufferMode = BACK_BUFFER;
				printf("BackBufferMode\n");
				selectMode = 1;
			}
			else {
				nowBufferMode = FRONT_BUFFER;
				printf("FrontBufferMode\n");
				selectMode = 0;
			}
		}
		else {
			if (isAutoRotating) isAutoRotating = false;
		}
	}

	////////////////////////////////////////modified//////////////////////////////////////////

    glutPostRedisplay();
}

void main( int argc, char *argv[] )
{
    width = 800;
    height = 600;
    frame = 0;
    glutInit( &argc, argv );						// Initialize openGL.
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB );	// Initialize display mode. This project will use double buffer and RGB color.
    glutInitWindowSize( width, height );			// Initialize window size.
    glutInitWindowPosition( 100, 100 );				// Initialize window coordinate.
    glutCreateWindow( "Simple Scene" );				// Create a window whose name is "Simple Scene".
    glutDisplayFunc( display );						// Register display function to be called for drawing screen event.
    glutReshapeFunc( reshape );						// Register reshape function to be called when window size is changed.
    glutKeyboardFunc( onKeyPress );					// Register onKeyPress function to be called when a keyboard input is provided.
    glutMouseFunc( onMouseButton );					// Register onMouseButton function to be called for mouse button events.
    glutMotionFunc( onMouseDrag );					// Register onMouseDrag function to be called that when a user drags mouse.
    int rv, gv, bv;
    glGetIntegerv( GL_RED_BITS, &rv );				// Get the depth of red bits from GL.
    glGetIntegerv( GL_GREEN_BITS, &gv );			// Get the depth of green bits from GL.
    glGetIntegerv( GL_BLUE_BITS, &bv );				// Get the depth of blue bits from GL.
    printf( "Pixel depth = %d : %d : %d\n", rv, gv, bv );
    initialize();									// Initialize a few other settings.
    glutMainLoop();									// Execute main loop which handles events.
}
