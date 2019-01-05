package org.usfirst.frc.team103.robot;

import java.util.ArrayList;
import java.util.List;

import org.usfirst.frc.team103.pixy.Pixy;
import org.usfirst.frc.team103.pixy.Pixy.ExposureSetting;
import org.usfirst.frc.team103.pixy.Pixy.WhiteBalanceSetting;

import edu.wpi.first.wpilibj.IterativeRobot;
import edu.wpi.first.wpilibj.Timer;
import edu.wpi.first.wpilibj.command.InstantCommand;
import edu.wpi.first.wpilibj.command.Scheduler;
import edu.wpi.first.wpilibj.smartdashboard.SmartDashboard;

public class Robot extends IterativeRobot {
	static final int FRAME_WIDTH = 320, FRAME_HEIGHT = 200;
	static final int MAX_BLOCKS = 10;

	Pixy pixy1, pixy2, pixy3;
	List<Pixy> pixies = new ArrayList<>();
	
	//[0xC6E0B552, 0x053C3165, 0xD892D58D] 
	@Override
	public void robotInit() {
		pixy1 = new Pixy(0x053C3165);
		pixy2 = new Pixy(0xC6E0B552);
		pixy3 = new Pixy(0xD892D58D);
		//pixies.add(pixy1);
		pixies.add(pixy2);
		//pixies.add(pixy3);
		//Pixy.ensureAvailable(0x053C3165, 0xC6E0B552, 0xD892D58D);
		Pixy.ensureAvailable(0xC6E0B552);
		SmartDashboard.putData("EnumeratePixies", new InstantCommand() {
			{
				setRunWhenDisabled(true);
			}
			@Override
			protected void initialize() {
				Pixy.enumerate();
				if (isEnabled()) {
					enabledInit();
				} else {
					disabledInit();
				}
			}
		});
	}
	@Override
	public void robotPeriodic() {
		Scheduler.getInstance().run();
	}
	
	@Override
	public void disabledInit() {
		for (Pixy pixy : pixies) {
			pixy.stopFrameGrabber();
			pixy.stopBlockProgram();
			pixy.setLEDBrightness(1000);
		}
	}
	@Override
	public void disabledPeriodic() {
		for (Pixy pixy : pixies) {
			double hue = (1.0 * Timer.getFPGATimestamp()) % 6.0;
			double x = 1.0 - Math.abs((hue % 2.0) - 1.0);
			double r, g, b;
			switch ((int) hue) {
			case 0:
				r = 1.0; g = x; b = 0.0; break;
			case 1:
				r = x; g = 1.0; b = 0.0; break;
			case 2:
				r = 0.0; g = 1.0; b = x; break;
			case 3:
				r = 0.0; g = x; b = 1.0; break;
			case 4:
				r = x; g = 0.0; b = 1.0; break;
			case 5:
				r = 1.0; g = 0.0; b = x; break;
			default:
				r = 0.0; g = 0.0; b = 0.0; break;
			}
			pixy.setLEDColor((int) (r * 255.0), (int) (g * 255.0), (int) (b * 255.0));
		}
	}
	
	public void enabledInit() {
		for (Pixy pixy : pixies) {
			pixy.setAutoWhiteBalance(false);
			pixy.setWhiteBalanceValue(new WhiteBalanceSetting(0xA0, 0x80, 0x80));
			pixy.setAutoExposure(false);
			pixy.setExposureCompensation(new ExposureSetting(10, 150));
			pixy.startBlockProgram();
			pixy.startFrameGrabber();
		}
	}
	public void enabledPeriodic() {
	}
	
	@Override
	public void autonomousInit() {
	}
	@Override
	public void autonomousPeriodic() {
	}

	@Override
	public void teleopInit() {
		enabledInit();
	}
	@Override
	public void teleopPeriodic() {
		enabledPeriodic();
	}

	@Override
	public void testInit() {
	}
	@Override
	public void testPeriodic() {
	}
}

