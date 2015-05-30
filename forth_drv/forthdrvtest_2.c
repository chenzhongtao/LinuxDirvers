
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>


/* forthdrvtest 
  */
int main(int argc, char **argv)
{
	int fd;
	unsigned char key_val;

	struct timeval tv = {1, 0};
	struct timeval *p_tv = NULL;
	fd_set rfds;

	int rc = 0;
	int s_rc = 0;
	int timeout = 5000;

	fd = open("/dev/buttons", O_RDWR);
	if (fd < 0)
	{
		printf("can't open!\n");
	}

	FD_ZERO(&rfds);						//����ļ�����������
	FD_SET(fd,&rfds);						//��fd����fds�ļ����������Դ�������select��������

	/*�����timeout��ms����ĵ�λ��������Ҫת��Ϊstruct timeval �ṹ��*/
	tv.tv_sec  = timeout / 1000;
	tv.tv_usec = (timeout%1000)*1000;

	p_tv = &tv;

	while (1)
	{
		s_rc = select(fd + 1, &rfds, NULL, NULL, p_tv);

		printf("sec: %d,usec: %d\n",(int)tv.tv_sec,(int)tv.tv_usec);
		
		printf("select s_rc = %d\n", s_rc);

		if(s_rc == 0)			/* ��ʱ */
		{
			/* ��ʱ֮��Ҫ���������ļ����������� */
			FD_ZERO(&rfds);						//����ļ�����������
			FD_SET(fd,&rfds);						//��fd����fds�ļ����������Դ�������select��������
			printf("time out\n");
		}
		else if(s_rc == 1)
		{
			rc = read(fd, &key_val, 1);
			printf("read %d byte. key_val = 0x%x\n", rc, key_val);
		}

		/* �������ó�ʱ */
		{
			tv.tv_sec  = timeout / 1000;
			tv.tv_usec = (timeout%1000)*1000;
			
			p_tv = &tv;	
		}	
	}
	
	return 0;
}

